#include "Clock.h"
#include "SynchroClockPins.h"
#include "unity.h"
#include "DLogPrintWriter.h"
#include "DS3231.h"

DLog&  dlog = DLog::getLog();
Clock clk(SYNC_PIN);
DS3231 ds;

void test_factory_reset()
{
    TEST_ASSERT_EQUAL(0, clk.factoryReset());
}

void test_begin()
{
    TEST_ASSERT_EQUAL(0, clk.begin(2));
}

void test_is_present()
{
    TEST_ASSERT(clk.isClockPresent());
}

void test_version()
{
    uint8_t version;
    TEST_ASSERT_EQUAL(0, clk.readVersion(& version));
    TEST_ASSERT_EQUAL_UINT8(1, version);
}

void test_position(uint16_t pos)
{
    TEST_ASSERT_EQUAL(0, clk.writePosition(pos));
    uint16_t position;
    TEST_ASSERT_EQUAL(0, clk.readPosition(&position));
    TEST_ASSERT_EQUAL(pos, position);
}

void test_position_0()
{
    test_position(0);
}

void test_position_half()
{
    test_position(CLOCK_MAX/2);
}

void test_position_max()
{
    test_position(CLOCK_MAX-1);
}

void test_adjust(uint16_t adj)
{
    TEST_ASSERT_EQUAL(0, clk.writePosition(0));
    TEST_ASSERT_EQUAL(0, clk.writeAdjustment(adj));
    delay(3000);
    uint16_t position;
    TEST_ASSERT_EQUAL(0, clk.readPosition(&position));
    TEST_ASSERT_EQUAL(10, position);
}

void test_adjust_10()
{
    test_adjust(10);
}

void test_reads()
{
    int errors = 0;
    uint16_t position;
    for (int i = 0; i < 5000; ++i)
    {
        errors += clk.readPosition(&position) == 0 ? 0 : 1;
        delay(1);
    }
    TEST_ASSERT_EQUAL(0, errors);
}

void setup()
{
    Wire.begin();
    Wire.setClockStretchLimit(1000000);
    ds.begin(); // need this for the tick signal
    delay(2000);
    UNITY_BEGIN();
    dlog.begin(new DLogPrintWriter(Serial));
    RUN_TEST(test_begin);
    delay(10);
    RUN_TEST(test_factory_reset);
    delay(1000);
    RUN_TEST(test_is_present);
    RUN_TEST(test_version);
    RUN_TEST(test_position_0);
    RUN_TEST(test_position_half);
    RUN_TEST(test_position_max);
    RUN_TEST(test_adjust_10);
    RUN_TEST(test_reads);
    UNITY_END();
}

void loop()
{
    delay(1000);
}
