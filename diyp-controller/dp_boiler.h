/* BOILER.h
 * Boiler
 * (c) 2025 diyPresso
 */
#ifndef BOILER_H
#define BOILER_H

#define _DP_FSM_TYPE BoilerStateMachine // used for the state machine macro NEXT()
#include "dp_hardware.h"
#include "dp_fsm.h"
#include "dp_pid.h"
#include "dp_heater.h"
#include <Arduino.h>

#include <MAX31865_NonBlocking.h> 


// Temperatures in [degC]
#define TEMP_WINDOW 10.0      // in temperature range
#define TEMP_LIMIT_HIGH 108.0 // > is TOO HGH
#define TEMP_LIMIT_LOW 1.0    // < is TOO LOW
#define TEMP_MIN_BREW 10.0    // do not brew under this temp

#define WINDUP_LIMIT_MIN -7.0 // windup limits in %
#define WINDUP_LIMIT_MAX 7.0  // 

// Times in [msec]
#define TIMEOUT_HEATING (600)    // maximum heater on time: 10 minutes
#define TIMEOUT_BREW (60 * 3)    // maximum brew on time: 3 minutes
#define TIMEOUT_READY (60 * 120) // maximum time in state ready: 2 hour

#define TIMEOUT_CONTROL_MSEC (1000 * 10)    // Max time between control updates [milliseconds]
#define TIMEOUT_HEATER_SSR_MSEC (1000 * 60) // maximum time the SSR is allowed to be ON [milliseconds]

// Safety: Temperature rate monitoring for dry boiler detection
#define TEMP_RATE_WINDOW_SEC (30)            // Time window for rate calculation [seconds] - balanced for 1300W element response
#define TEMP_RATE_MAX_NORMAL (5.0)           // Max normal temperature rise rate [degC/min]
#define TEMP_RATE_MAX_DRY (25.0)             // Max safe temperature rise rate [degC/min] - tuned for 1300W element

// Boiler level check reasons
typedef enum {
  BOILER_CHECK_NONE,
  BOILER_CHECK_STARTUP,
  BOILER_CHECK_PRESLEEP,
  BOILER_CHECK_POSTBREW,
  BOILER_CHECK_EMERGENCY
} boiler_check_reason_t;

// Various boiler errors
typedef enum
{
  BOILER_ERROR_NONE,
  BOILER_ERROR_RTD,
  BOILER_ERROR_TIMEOUT_BREW,
  BOILER_ERROR_TIMEOUT_HEATING,
  BOILER_ERROR_READY_TIMEOUT,
  BOILER_ERROR_SSR_TIMEOUT,
  BOILER_ERROR_OVER_TEMP,
  BOILER_ERROR_UNDER_TEMP,
  BOILER_ERROR_CONTROL_TIMEOUT,
  BOILER_ERROR_DRY_BOILER,
  BOILER_ERROR_UNKNOWN,
} boiler_error_t;

class BoilerStateMachine : public StateMachine<BoilerStateMachine>
{
public:
  BoilerStateMachine() : StateMachine(&BoilerStateMachine::state_off) {}; // moved intit() out of the constructor, because the arduino just bricked if called earlier. Not sure why though...
  int error() { return _error; }
  void clear_error() { _error = BOILER_ERROR_NONE; }
  double set_temp() { return _set_temp; }
  double set_temp(double temp) { 
    double new_temp = min(TEMP_LIMIT_HIGH, max(temp, 0.0));
    // If temperature change is significant and we're in ready state, force recheck
    if (abs(new_temp - _set_temp) > TEMP_WINDOW && _cur_state == &BoilerStateMachine::state_ready) {
      _force_state_recheck = true;
    }
    return _set_temp = new_temp; 
  }
  double act_temp() { return _act_temp; }
  double act_power() { return _power; }
  double set_ff_heat(double ff) { return _ff_heat = min(100.0, max(ff, 0.0)); }
  double get_ff_heat(void) { return _ff_heat; }
  double set_ff_ready(double ff) { return _ff_ready = min(100.0, max(ff, 0.0)); }
  double get_ff_ready(void) { return _ff_ready; }
  double set_ff_brew(double ff) { return _ff_brew = min(100.0, max(ff, 0.0)); }
  double get_ff_brew(void) { return _ff_brew; }
  void set_pid(double p, double i, double d) { _pid.setCoefficients(p, i, d); }
  void on() { _on = true; }
  void off()
  {
    _on = false;
    heaterDevice.power(0);
  }
  void start_brew() { _brew = true; }
  void stop_brew() { _brew = false; }
  void force_state_recheck() { _force_state_recheck = true; }
  bool is_on() { return _on; }
  bool is_ready() { return _cur_state == &BoilerStateMachine::state_ready; }
  bool is_error() { return _cur_state == &BoilerStateMachine::state_error; }
  const char *get_error_text();
  const char *get_state_name();
  void control();
  void begin();
  void init(); 
  
  // Boiler level checking
  void request_boiler_check(boiler_check_reason_t reason);
  bool is_boiler_check_pending() { return _boiler_check_in_progress; }
  void process_boiler_level_check();
  
#ifdef SIMULATE
  // Public helpers for simulation
  void set_sim_temp_override(double temp) { _sim_temp_override = temp; }
  void clear_sim_temp_override() { _sim_temp_override = -1.0; }
#endif

private:
  DpPID _pid;
  double _act_temp = 0, _set_temp = 0, _ff_heat = 0, _ff_ready = 0, _ff_brew = 0, _power = 0;
  bool _on = false, _brew = false;
  unsigned long _last_control_time = 0;
  boiler_error_t _error = BOILER_ERROR_NONE;
  int _rtd_error = 0;   // current RTD errors

  // Temperature rate monitoring for dry boiler detection
  double _temp_rate_history[TEMP_RATE_WINDOW_SEC] = {0}; // Rolling average buffer
  int _temp_rate_index = 0;
  unsigned long _last_temp_time = 0;
  double _prev_temp = 0;

  // Simulation override for testing
  double _sim_temp_override = -1.0;
  
  // Boiler level checking
  boiler_check_reason_t _pending_boiler_check = BOILER_CHECK_NONE;
  bool _boiler_check_in_progress = false;
  double _boiler_check_start_weight = 0;
  unsigned long _boiler_check_start_time = 0;
  
  // State recheck flag
  bool _force_state_recheck = false;
  
  void state_off();     // SSR is forced OFF
  void state_heating(); // Temperature control, but not yet on target temperature
  void state_ready();   // temperature control, within range of target temperature
  void state_brew();    // temperature control in brewing mode with feed-forward active
  void state_error();   // heater is forced OFF, error code is set, set state to OFF to clear error
  void goto_error(boiler_error_t err);
  void check_dry_boiler_safety();
  void start_boiler_level_check();
  void handle_boiler_check_result(bool was_full);
  const char* get_check_reason_text(boiler_check_reason_t reason);
  MAX31865 thermistor = MAX31865(PIN_THERM_CS);
};

extern BoilerStateMachine boilerController;

#endif // BOILER_H 