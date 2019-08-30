#include "Clock.h"
#include "SynchroClockPins.h"
#include "unity.h"

DLog&  dlog = DLog::getLog();
Clock clk(SYNC_PIN);

void test_begin()
{
    TEST_ASSERT_EQUAL(0, clk.begin(1));
}


#define READ_COUNT 5000
void test_read_version()
{
    uint8_t version;
    uint32_t read_errors = 0;
    int32_t min_read_time = -1;
    int32_t max_read_time = -1;
    for (int i = 0; i<READ_COUNT; ++i)
    {
        uint32_t start = millis();
        read_errors += clk.readVersion(& version) == 0 ? 0 : 1;
        uint32_t duration = millis() - start;
        if (min_read_time == -1 || (int)duration < min_read_time)
        {
            min_read_time = duration;
        }
        if (max_read_time == -1 || (int)duration > max_read_time)
        {
            max_read_time = duration;
        }
        if (version != 1)
        {
            Serial.printf("read_ver: %d invalid version: %u\n", i, version);
        }
        if ((i % 1000) == 0)
        {
            Serial.printf("read_ver: %d errors: %u min_ms: %d max_ms: %d\n", i, read_errors, min_read_time, max_read_time);
        }
        delay(1);
    }
    Serial.printf("read_ver: %d errors: %u min_ms: %d max_ms: %d\n", READ_COUNT, read_errors, min_read_time, max_read_time);
    TEST_ASSERT_LESS_OR_EQUAL(20, read_errors);
}

void test_read_position()
{
    uint16_t position;
    uint32_t read_errors = 0;
    int32_t min_read_time = -1;
    int32_t max_read_time = -1;
    for (int i = 0; i<READ_COUNT; ++i)
    {
        position = 0;
        uint32_t start = millis();
        read_errors += clk.readPosition(& position) == 0 ? 0 : 1;
        uint32_t duration = millis() - start;
        if (min_read_time == -1 || (int)duration < min_read_time)
        {
            min_read_time = duration;
        }
        if (max_read_time == -1 || (int)duration > max_read_time)
        {
            max_read_time = duration;
        }
        if (position > CLOCK_MAX)
        {
            Serial.printf("read_pos: %d invalid position: %u\n", i, position);
        }
        if ((i % 1000) == 0)
        {
            Serial.printf("read_pos: %d errors: %u min_ms: %d max_ms: %d\n", i, read_errors, min_read_time, max_read_time);
        }
        delay(1);
    }
    Serial.printf("read_pos: %d errors: %u min_ms: %d max_ms: %d\n", READ_COUNT, read_errors, min_read_time, max_read_time);
    TEST_ASSERT_LESS_OR_EQUAL(20, read_errors);
}

void setup()
{
    Wire.begin();
    Wire.setClockStretchLimit(1000000);
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_begin);
    delay(10);
    RUN_TEST(test_read_position);
    RUN_TEST(test_read_version);
    UNITY_END();
}

void loop()
{
    delay(1000);
}
