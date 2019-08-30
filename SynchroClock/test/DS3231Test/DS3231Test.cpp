#include "DS3231.h"
#include "SynchroClockPins.h"
#include "unity.h"

#define NO_RETRIES
//#define VERBOSE_OUTPUT

DLog&  dlog = DLog::getLog();
DS3231 ds;

#ifndef NO_RETRIES
int clear_bus_count;
void clearBus()
{
    ++clear_bus_count;
    Serial.printf("ClearBus: %d\n", clear_bus_count);
    WireUtils.clearBus();
}
#endif

#define EDGE_FALLING 0
#define EDGE_RISING 1

//
// wait for an "edge" of the 1hz signal from the ds3231
//
uint32_t waitForEdge(int edge, uint32_t timeout, uint32_t &start)
{
    uint32_t duration = 0;
    while(digitalRead(SYNC_PIN) != edge)
    {
        duration = millis() - start;
        if (duration > timeout)
        {
            return duration;
        }
        delay(0);
    }
    uint32_t end = millis();
    duration =  end - start;
    start = end;
    return duration;
}

#define MIN_MS 499
#define MAX_MS 501

void test_1hz()
{
    uint32_t start = millis();
    //
    // The first 2 waits are to sync with the 1hz signal and we can only test for too long a wait
    //
    uint32_t duration = waitForEdge(EDGE_RISING, MAX_MS, start);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_MS, duration); // can't be longer than max
    duration = waitForEdge(EDGE_FALLING, MAX_MS, start);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_MS, duration); // can't be longer than max
    //
    // Now we can test for min and max wait times
    //
    duration = waitForEdge(EDGE_RISING, MAX_MS, start);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_MS, duration);
    TEST_ASSERT_GREATER_OR_EQUAL(MIN_MS, duration);
    duration = waitForEdge(EDGE_FALLING, MAX_MS, start);
    TEST_ASSERT_LESS_OR_EQUAL(MAX_MS, duration);
    TEST_ASSERT_GREATER_OR_EQUAL(MIN_MS, duration);
}

void test_begin()
{
    TEST_ASSERT_EQUAL(0, ds.begin());
}

int readTime(DS3231DateTime& dt, unsigned int retries)
{
#ifdef NO_RETRIES
    (void)retries;
    return ds.readTime(dt);
#else
    while(retries-- > 0)
    {
        if (ds.readTime(dt) == 0)
        {
            return 0;
        }
        clearBus();
    }
    return -1;
#endif
}

#define READ_COUNT 10000

void test_read()
{
    DS3231DateTime dt;
    uint32_t read_errors = 0;
    uint32_t min_read_time = 0;
    uint32_t max_read_time = 0;
    for (int i = 0; i<READ_COUNT; ++i)
    {
        uint32_t start = millis();
        read_errors += readTime(dt, 3);
        uint32_t duration = millis() - start;
        if (min_read_time == 0 || duration < min_read_time)
        {
            min_read_time = duration;
        }
        if (max_read_time == 0 || duration > max_read_time)
        {
            max_read_time = duration;
        }
        delay(0);
    }
    Serial.printf("read: %d errors: %u min_ms: %u max_ms: %u\n", READ_COUNT, read_errors, min_read_time, max_read_time);
    TEST_ASSERT_EQUAL(0, read_errors);
}

void setup()
{
    Wire.begin();
    Wire.setClockStretchLimit(100000);
    pinMode(SYNC_PIN, INPUT);
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_begin);
    RUN_TEST(test_1hz);
    RUN_TEST(test_read);
#ifndef NO_RETRIES
    if (clear_bus_count >= 0)
    {
        Serial.printf("Total cleanBus: %d\n", clear_bus_count);
        clear_bus_count = -1;
    }
#endif
    UNITY_END();
}

void loop()
{
    delay(1000);
}
