# Safety Features Testing Guide

This document provides instructions for testing the safety improvements made to prevent dry boiler operation.

## Testing Environment Setup

### 1. Build the Simulation Version

```bash
# Build the simulation version (no hardware required)
pio run -e mkr_wifi1010_simulate

# Upload to board (or test in simulation)
pio run -e mkr_wifi1010_simulate --target upload
```

### 2. Enable Simulation Mode

The simulation mode is automatically enabled when building with the `mkr_wifi1010_simulate` environment.

## Safety Features to Test

### Feature 1: Water Verification During Commissioning

**What it tests:** Ensures minimum 125ml (125g) water is pumped before allowing commissioning completion.

**How to test:**
1. Start with empty reservoir simulation
2. Press encoder button to start commissioning
3. Pump should run for 30 seconds
4. If less than 125g water detected: should show "NO_FILL" error
5. If 125g+ water detected: proceeds to purge state

**Expected behavior:**
- With insufficient water: Commissioning fails with "NO_FILL" error
- With sufficient water: Commissioning completes successfully

### Feature 2: Dry Boiler Detection

**What it tests:** Detects when boiler temperature rises too quickly (indicating dry operation).

**How to test:**
1. Start normal operation
2. Press encoder button **3 times quickly** (within 2 seconds) to enter test mode
3. Wait 30 seconds for normal heating
4. System simulates rapid temperature rise (120°C/min)
5. Should detect dry boiler condition and shut down

**Expected behavior:**
- Normal heating: gradual temperature rise
- Dry boiler simulation: rapid temperature rise triggers "DRY_BOILER" error
- System shuts down heating automatically

## Test Scenarios

### Scenario 1: Normal Commissioning (PASS)
```
1. Empty reservoir (weight = 0)
2. Press button → start commissioning
3. Pump runs 30 seconds
4. Simulate 150g water transfer
5. System detects sufficient water
6. Commissioning completes successfully
```

### Scenario 2: Insufficient Water (FAIL)
```
1. Empty reservoir (weight = 0)
2. Press button → start commissioning
3. Pump runs 30 seconds
4. Simulate only 50g water transfer
5. System detects insufficient water
6. Shows "NO_FILL" error
7. Commissioning fails
```

### Scenario 3: Dry Boiler Detection (FAIL)
```
1. Enter test mode (3x button press)
2. Normal heating for 30 seconds
3. Simulate rapid temperature rise
4. System detects abnormal heating rate
5. Shows "DRY_BOILER" error
6. Heating shuts down automatically
```

## Serial Output Monitoring

Connect to serial monitor to observe test progress:

```bash
pio device monitor
```

**Normal operation:**
```
reservoir_level: 85.2, boiler_temp: 45.2/93.0, boiler_power: 75.3
brew_state: idle, boiler_state: heating
```

**Test mode activation:**
```
=== ENTERING SAFETY TEST MODE ===
Testing water verification and dry boiler detection
TEST: Normal heating - temp should rise gradually
TEST: Simulating dry boiler - rapid temp rise
DRY_BOILER error should trigger soon...
```

**Error conditions:**
```
boiler_error=DRY_BOILER
brew_error=NO_FILL
```

## Manual Testing Commands

### Via Serial Commands
- `brew start` - Start brewing process
- `boiler on/off` - Control boiler heating
- `reservoir tare` - Reset reservoir weight

### Via MQTT (if enabled)
- Topic: `diyPresso/command`
- Payload: `{"cmd": "brew_start"}`

## Verification Checklist

- [ ] Water verification prevents dry commissioning
- [ ] Dry boiler detection shuts down heating
- [ ] Error messages are clear and informative
- [ ] System recovers properly after errors
- [ ] Serial logging provides adequate debugging info

## Troubleshooting

**Test mode doesn't activate:**
- Ensure encoder button presses are within 2 seconds
- Check serial output for button detection

**Simulation doesn't work:**
- Verify SIMULATE flag is defined
- Check PlatformIO environment selection
- Ensure correct board configuration

**Errors not triggering:**
- Check temperature rate calculation logic
- Verify error handling in state machine
- Confirm safety thresholds are appropriate

## Safety Thresholds

```cpp
// Water verification
#define FILL_WEIGHT_DROP_MINIMUM (125.0) // grams (~125ml)

// Temperature rate monitoring
#define TEMP_RATE_MAX_NORMAL (5.0)      // degC/min normal operation
#define TEMP_RATE_MAX_DRY (25.0)        // degC/min triggers shutdown (tuned for 1300W element)
#define TEMP_RATE_WINDOW_SEC (30)       // seconds for rate calculation (faster response)
```

These thresholds can be adjusted based on testing results and specific hardware characteristics.

