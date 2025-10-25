/*
 * diyPresso global header file
 */
#ifndef DP_H
#define DP_H

#include <Arduino.h>

// #define SIMULATE // Define this to compile as SIMULATED device (no hardware)
#define WATCHDOG_ENABLED // if not defined: Watchdog is disabled! ENABLE FOR PRODUCTION!!!!

#define AUTOSLEEP_TIMEOUT (60 * 60.0)   // [sec] When longer than this time in idle, goto sleep
#define SHUTDOWN_TIMEOUT (4 * 60 * 60.0) // [sec] When longer than this time in sleep, shutdown (4 hours)
#define INITIAL_PUMP_TIME (30.0)        // time to pump at startup [sec]
#define FILL_WEIGHT_DROP_MINIMUM (125.0) // Minimum Weight drop after filling boiler [grams] - ~125ml water
#define PURGE_TIMEOUT (30.0)
#define PURGE_WEIGHT_DROP_MINIMUM (50.0) // Minimum weight drop after purging [gram]

// Boiler level checking
#define BOILER_FILL_TEST_TIME_SEC (5)      // Test pump for 5 seconds
#define BOILER_FILL_THRESHOLD_G (10)       // Minimum weight change to detect
#define BOILER_FILL_MAX_TIME_SEC (60)      // Maximum total fill time (safety timeout)

#define SOFTWARE_VERSION "1.9.0-dev"
#define BUILD_DATE __DATE__ " " __TIME__

#endif // DP_H