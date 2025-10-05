/*
  Time functions with millis() overflow protection
  Handles the 49.7-day millis() overflow gracefully
 */
#include <Arduino.h>

const ulong UL_MAX = 4294967295;

// return difference between two timestamps, where `ts1` is larger than `ts2`, handles wraparound
unsigned long time_diff(unsigned long ts1, unsigned long ts2)
{
    if (ts1 < ts2) // Handle overflow
		return (UL_MAX - ts2) + ts1;
	else
		return ts1 - ts2;
}

// return time since timestamp in msec 
unsigned long time_since(unsigned long ts)
{
    return time_diff(millis(), ts);
}

// return time since timestamp in microseconds 
unsigned long usec_since(unsigned long ts)
{
    return time_diff(micros(), ts);
}

// return true/false, period approx 1 sec
bool blink ()
{
    return (millis() >> 9) & 1;
}

// Safe timeout check - handles millis() overflow
bool timeout_elapsed(unsigned long start_time, unsigned long timeout_ms)
{
    return time_since(start_time) >= timeout_ms;
}

// Safe elapsed time calculation - handles millis() overflow
unsigned long elapsed_time(unsigned long start_time)
{
    return time_since(start_time);
}

// Safe timeout check for seconds (converts to ms internally)
bool timeout_elapsed_sec(unsigned long start_time, double timeout_sec)
{
    return timeout_elapsed(start_time, (unsigned long)(timeout_sec * 1000.0));
}

#ifdef TEST_MILLIS_OVERFLOW
// Test functions for millis() overflow simulation
static unsigned long simulated_millis = 0;
static bool overflow_test_mode = false;

// Override millis() for testing
unsigned long test_millis()
{
    if (overflow_test_mode) {
        return simulated_millis;
    }
    return millis();
}

// Override time_since for testing
unsigned long test_time_since(unsigned long ts)
{
    return time_diff(test_millis(), ts);
}

// Set simulated millis value for testing
void set_test_millis(unsigned long value)
{
    simulated_millis = value;
    overflow_test_mode = true;
}

// Reset to real millis()
void reset_test_millis()
{
    overflow_test_mode = false;
}

// Test overflow scenarios
void test_millis_overflow()
{
    Serial.println("\n=== TESTING MILLIS() OVERFLOW PROTECTION ===");
    
    // Test 1: Normal operation (no overflow)
    unsigned long start = 1000;
    set_test_millis(start);
    unsigned long test_start = test_millis();
    
    set_test_millis(start + 5000);
    unsigned long elapsed = test_time_since(test_start);
    Serial.print("Test 1 - Normal: elapsed=");
    Serial.print(elapsed);
    Serial.println(elapsed == 5000 ? " ✓ PASS" : " ✗ FAIL");
    
    // Test 2: Overflow scenario
    start = UL_MAX - 2000;  // Near overflow
    set_test_millis(start);
    test_start = test_millis();
    
    set_test_millis(1000);  // After overflow
    elapsed = test_time_since(test_start);
    Serial.print("Test 2 - Overflow: elapsed=");
    Serial.print(elapsed);
    Serial.println(elapsed == 3000 ? " ✓ PASS" : " ✗ FAIL");
    
    // Test 3: Timeout functions
    start = UL_MAX - 1000;
    set_test_millis(start);
    test_start = test_millis();
    
    set_test_millis(2000);  // After overflow
    bool timeout = test_time_since(test_start) >= 3000;
    Serial.print("Test 3 - Timeout: ");
    Serial.println(timeout ? " ✓ PASS" : " ✗ FAIL");
    
    // Test 4: Multiple overflow cycles
    start = UL_MAX - 500;
    set_test_millis(start);
    test_start = test_millis();
    
    set_test_millis(UL_MAX + 1000);  // Multiple overflows
    elapsed = test_time_since(test_start);
    Serial.print("Test 4 - Multiple overflows: elapsed=");
    Serial.print(elapsed);
    Serial.println(elapsed == 1500 ? " ✓ PASS" : " ✗ FAIL");
    
    reset_test_millis();
    Serial.println("=== OVERFLOW TEST COMPLETE ===\n");
}
#endif
