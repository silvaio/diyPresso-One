#ifndef TIME_H
#define TIME_H
#include <Arduino.h>

unsigned long time_diff(unsigned long ts1, unsigned long ts2);
unsigned long time_since(unsigned long ts);
unsigned long usec_since(unsigned long usec_ts);

extern bool blink();

// Safe timing functions with millis() overflow protection
bool timeout_elapsed(unsigned long start_time, unsigned long timeout_ms);
unsigned long elapsed_time(unsigned long start_time);
bool timeout_elapsed_sec(unsigned long start_time, double timeout_sec);

#ifdef TEST_MILLIS_OVERFLOW
// Test functions for overflow simulation
unsigned long test_millis();
void set_test_millis(unsigned long value);
void reset_test_millis();
void test_millis_overflow();
#endif

#endif

