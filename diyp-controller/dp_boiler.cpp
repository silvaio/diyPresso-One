/*
  boiler.cpp
  diyPresso Boiler control
  (c) 2024 DiyEspresso - PBRI - CC-BY-NC
 */
#include "dp.h"
#include "dp_hardware.h"
#include "dp_boiler.h"
#include "dp_heater.h"
#include "dp_settings.h"
#include "dp_time.h"  // Include timing functions
#include "dp_reservoir.h"  // For reservoir extern
#include "dp_pump.h"      // For pumpDevice extern

//#include <Adafruit_MAX31865.h>

#ifdef WATCHDOG_ENABLED
#include <wdt_samd21.h>
#endif

BoilerStateMachine boilerController = BoilerStateMachine();

void BoilerStateMachine::state_off()
{
  ;
  if (_on)
    NEXT(state_heating);
}

void BoilerStateMachine::state_heating()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_heat);
  }
  if (!_on)
    NEXT(state_off);
  if (_brew)
    NEXT(state_brew);
  if (abs(_set_temp - _act_temp) < TEMP_WINDOW)
    NEXT(state_ready);
  ON_TIMEOUT_SEC(TIMEOUT_HEATING)
  goto_error(BOILER_ERROR_TIMEOUT_HEATING);
  ON_EXIT()
  {
    _pid.setFeedForward(0);
  }
}

void BoilerStateMachine::state_ready()
{
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_ready);
  }
  if (!_on)
    NEXT(state_off);
  if (_brew)
    NEXT(state_brew);
  if (abs(_set_temp - _act_temp) > TEMP_WINDOW)
    NEXT(state_heating);
  if (_force_state_recheck)
  {
    _force_state_recheck = false;
    // Force re-evaluation of temperature difference
    if (abs(_set_temp - _act_temp) > TEMP_WINDOW)
      NEXT(state_heating);
  }
  ON_TIMEOUT_SEC(TIMEOUT_READY)
  goto_error(BOILER_ERROR_READY_TIMEOUT);
}

void BoilerStateMachine::state_brew()
{
  if (!_on)
    NEXT(state_off);
  if (!_brew)
    NEXT(state_heating);
  ON_ENTRY()
  {
    _pid.setFeedForward(_ff_brew);
  }

  // if ( (_set_temp - _act_temp ) > TEMP_WINDOW) goto_error(BOILER_ERROR_UNDER_TEMP);
  ON_TIMEOUT_SEC(TIMEOUT_BREW)
  goto_error(BOILER_ERROR_TIMEOUT_BREW);
  ON_EXIT()
  {
    _pid.setFeedForward(0);
    _brew = false;
  }
}

void BoilerStateMachine::state_error()
{
  off();
  _power = 0;
  _set_temp = 0;
  if (_error == BOILER_ERROR_NONE)
    NEXT(state_off);
}

void BoilerStateMachine::goto_error(boiler_error_t error)
{
  _error = error;
  NEXT(state_error);
}

void BoilerStateMachine::init()
{
  _pid.begin(&_act_temp, &_power, &_set_temp, settings.P(), settings.I(), settings.D(), settings.ff_ready(), 1000); // get defaults from setting and set PID sample time to 1s (same as HeaterDevice)
  _pid.setOutputLimits(0, 100);
  _pid.setWindUpLimits(WINDUP_LIMIT_MIN, WINDUP_LIMIT_MAX); // set bounds for the integral term to prevent integral wind-up
  _pid.start();

  begin();  // start the thermistor.
  delay(500); // wait for the thermistor to start up. TODO: wait in loop?
  _error = BOILER_ERROR_NONE;
  _rtd_error = 0;
  _on = true;
  _last_control_time = millis();
  
  // Initialize watchdog after everything else is ready
#ifdef WATCHDOG_ENABLED
  delay(100); // Small delay to ensure system is stable
  wdt_init(WDT_CONFIG_PER_16K);
#endif
}

void BoilerStateMachine::begin()
{
  thermistor.begin(MAX31865::RTD_2WIRE, MAX31865::FILTER_50HZ, MAX31865:: CONV_MODE_CONTINUOUS); // set to 2WIRE, default filter and continuous conversion mode.
}



void BoilerStateMachine::control(void)
{

  //unsigned long start_time = millis();
  //_act_temp = thermistor.temperature(RNOMINAL, RREF);
  _act_temp = thermistor.getTemperature(RNOMINAL, RREF);

#ifdef SIMULATE
  // Use simulation override if set, otherwise use average power as temperature
  if (_sim_temp_override >= 0.0) {
    _act_temp = _sim_temp_override;
  } else {
    _act_temp = heaterDevice.average(); // hack for testing, read average power as actual temperature
  }
#endif

  //_rtd_error = thermistor.readFault();
  _rtd_error = thermistor.getFault();
  if (_rtd_error)
  {
    thermistor.clearFault();
    goto_error(BOILER_ERROR_RTD);
  }
  else
  {
    if (_act_temp > TEMP_LIMIT_HIGH)
      goto_error(BOILER_ERROR_OVER_TEMP);
    if (_act_temp < TEMP_LIMIT_LOW)
      goto_error(BOILER_ERROR_RTD);

    // Check for dry boiler condition
    check_dry_boiler_safety();
  }

  if (_on && timeout_elapsed(_last_control_time, TIMEOUT_CONTROL_MSEC))
    goto_error(BOILER_ERROR_CONTROL_TIMEOUT);
  _last_control_time = millis();

  run();

  // Process boiler level checking
  process_boiler_level_check();

  _pid.compute();

  // char buffer[10];
  // Serial.print("Diff: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power2 - _power);
  // Serial.print(buffer);
  // Serial.print("PID1: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power);
  // Serial.print(buffer);
  // Serial.print(" PID2: ");
  // snprintf(buffer, sizeof(buffer), "%6.1f", _power2);
  // Serial.print(buffer);
  // Serial.print(" Temp: ");
  // Serial.print(_act_temp);
  // Serial.print("/");
  // Serial.print(_set_temp);
  // Serial.print(" P: ");
  // Serial.print(_pid.P());
  // Serial.print("/");
  // Serial.print(_pid2.P());
  // Serial.print(" I: ");
  // Serial.print(_pid.I());
  // Serial.print("/");
  // Serial.print(_pid2.I());
  // Serial.print(" D: ");
  // Serial.print(_pid.D());
  // Serial.print("/");
  // Serial.println(_pid2.D());

  if (_act_temp > (TEMP_LIMIT_HIGH + 2.0))
    _power = 0;
  heaterDevice.power(_on ? _power : 0.0);
#ifdef WATCHDOG_ENABLED
  wdt_reset();
#endif

}

const char *BoilerStateMachine::get_error_text()
{
  switch (_error)
  {
  case BOILER_ERROR_NONE:
    return "OK";
  case BOILER_ERROR_OVER_TEMP:
    return "OVER_TEMP";
  case BOILER_ERROR_UNDER_TEMP:
    return "UNDER_TEMP";
  case BOILER_ERROR_RTD:
    return "RTD_ERROR";
  case BOILER_ERROR_SSR_TIMEOUT:
    return "SSR_TIMEOUT";
  case BOILER_ERROR_TIMEOUT_BREW:
    return "BREW_TIMEOUT";
  case BOILER_ERROR_CONTROL_TIMEOUT:
    return "CONTROL_TIMEOUT";
  case BOILER_ERROR_DRY_BOILER:
    return "DRY_BOILER";
  case BOILER_ERROR_READY_TIMEOUT:
    return "READY_TIMEOUT";
  case BOILER_ERROR_TIMEOUT_HEATING:
    return "TIMEOUT_HEATING";
  default:
    return "UNKNOWN";
  }
}

void BoilerStateMachine::check_dry_boiler_safety()
{
  unsigned long current_time = millis();

  // Only check during heating phases and when we have enough data
  if (!_on || _brew || _temp_rate_index < 10)
    return;

  // Calculate temperature rate (degC per minute)
  if (_last_temp_time > 0) {
    double time_diff_sec = (current_time - _last_temp_time) / 1000.0;
    if (time_diff_sec > 0) {
      double temp_diff = _act_temp - _prev_temp;
      double rate_per_min = (temp_diff / time_diff_sec) * 60.0;

      // Store rate in circular buffer
      _temp_rate_history[_temp_rate_index % TEMP_RATE_WINDOW_SEC] = rate_per_min;
      _temp_rate_index++;

      // Calculate average rate over the window
      double avg_rate = 0;
      int samples = min(_temp_rate_index, TEMP_RATE_WINDOW_SEC);
      for (int i = 0; i < samples; i++) {
        avg_rate += _temp_rate_history[i];
      }
      avg_rate /= samples;

      // Check for dangerously high heating rate (dry boiler)
      if (avg_rate > TEMP_RATE_MAX_DRY && _act_temp > 50.0) {
        // Trigger emergency boiler check before error
        request_boiler_check(BOILER_CHECK_EMERGENCY);
        goto_error(BOILER_ERROR_DRY_BOILER);
        return;
      }
    }
  }

  // Update for next iteration
  _prev_temp = _act_temp;
  _last_temp_time = current_time;
}

const char *BoilerStateMachine::get_state_name()
{
  RETURN_STATE_NAME(off);
  RETURN_STATE_NAME(heating);
  RETURN_STATE_NAME(ready);
  RETURN_STATE_NAME(brew);
  RETURN_STATE_NAME(error);
  RETURN_NONE_STATE_NAME()
  RETURN_UNKNOWN_STATE_NAME();
}

// Boiler level checking implementation
void BoilerStateMachine::request_boiler_check(boiler_check_reason_t reason)
{
  if (!_boiler_check_in_progress && !_brew) {
    _pending_boiler_check = reason;
    start_boiler_level_check();
  }
}

void BoilerStateMachine::start_boiler_level_check()
{
  _boiler_check_in_progress = true;
  _boiler_check_start_weight = reservoir.weight();
  _boiler_check_start_time = millis();
  pumpDevice.on();
  
  Serial.print("Boiler check started: ");
  Serial.println(get_check_reason_text(_pending_boiler_check));
}

void BoilerStateMachine::process_boiler_level_check()
{
  if (!_boiler_check_in_progress) return;
  
  unsigned long elapsed = millis() - _boiler_check_start_time;
  
  // Safety timeout - stop filling after maximum time
  if (elapsed >= BOILER_FILL_MAX_TIME_SEC * 1000) {
    pumpDevice.off();
    Serial.print("Boiler fill timeout after ");
    Serial.print(BOILER_FILL_MAX_TIME_SEC);
    Serial.println(" seconds - stopping");
    handle_boiler_check_result(false);
    _boiler_check_in_progress = false;
    _pending_boiler_check = BOILER_CHECK_NONE;
    return;
  }
  
  if (elapsed >= BOILER_FILL_TEST_TIME_SEC * 1000) {
    // Check completed
    double weight_change = _boiler_check_start_weight - reservoir.weight();
    pumpDevice.off();
    
    bool boiler_was_full = (weight_change < BOILER_FILL_THRESHOLD_G);
    
    Serial.print("Boiler check completed: ");
    Serial.print(get_check_reason_text(_pending_boiler_check));
    Serial.print(" - ");
    Serial.println(boiler_was_full ? "FULL" : "REFILLED");
    
    if (boiler_was_full) {
      // Boiler is full - we're done
      handle_boiler_check_result(true);
      _boiler_check_in_progress = false;
      _pending_boiler_check = BOILER_CHECK_NONE;
    } else {
      // Boiler wasn't full - continue filling
      Serial.println("Boiler not full yet - continuing to fill...");
      _boiler_check_start_weight = reservoir.weight();  // Reset baseline
      _boiler_check_start_time = millis();               // Reset timer
      pumpDevice.on();                                   // Keep pumping
    }
  }
}

void BoilerStateMachine::handle_boiler_check_result(bool was_full)
{
  switch (_pending_boiler_check) {
    case BOILER_CHECK_STARTUP:
      if (!was_full) {
        Serial.println("Startup: Boiler was empty - refilled");
      }
      break;
      
    case BOILER_CHECK_PRESLEEP:
      if (!was_full) {
        Serial.println("Pre-sleep: Boiler was empty - refilled");
      }
      // Now safe to proceed with sleep
      break;
      
    case BOILER_CHECK_POSTBREW:
      if (!was_full) {
        Serial.println("Post-brew: Boiler was empty - refilled");
      }
      break;
      
    case BOILER_CHECK_EMERGENCY:
      if (!was_full) {
        Serial.println("Emergency: Boiler was empty - refilled");
      }
      break;
      
    default:
      break;
  }
}

const char* BoilerStateMachine::get_check_reason_text(boiler_check_reason_t reason)
{
  switch (reason) {
    case BOILER_CHECK_NONE: return "NONE";
    case BOILER_CHECK_STARTUP: return "STARTUP";
    case BOILER_CHECK_PRESLEEP: return "PRESLEEP";
    case BOILER_CHECK_POSTBREW: return "POSTBREW";
    case BOILER_CHECK_EMERGENCY: return "EMERGENCY";
    default: return "UNKNOWN";
  }
}