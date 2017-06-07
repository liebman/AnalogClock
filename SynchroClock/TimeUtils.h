/*
 * TimeUtils.h
 *
 *  Created on: Jun 7, 2017
 *      Author: liebman
 */

#ifndef TIMEUTILS_H_
#define TIMEUTILS_H_
#include <Arduino.h>
#include <time.h>
#include "Logger.h"

#define DEBUG_TIME_UTILS

typedef struct
{
    int     tz_offset;     // seconds offset from UTC
    uint8_t month;         // starting month that this takes effect.
    int8_t  occurrence;    // 2 is second occurrence of the given day, -1 is last
    uint8_t day_of_week;   // 0 = Sunday
    uint8_t hour;
} TimeChange;

class TimeUtils
{
public:
    static uint8_t    parseSmallDuration(const char* value);
    static int        parseOffset(const char* offset_string);
    static uint16_t   parsePosition(const char* position_string);
    static time_t     mktime(struct tm *tmbuf);
    static struct tm* gmtime_r(const time_t *timer, struct tm *tmbuf);
    static int        computeUTCOffset(uint16_t year, uint8_t month, uint8_t wday, uint8_t hour, TimeChange* tc, int tc_count);
    static uint8_t    findDOW(uint16_t y, uint8_t m, uint8_t d);
    static uint8_t    findNthDate(uint16_t year, uint8_t month, uint8_t dow, uint8_t nthWeek);
    static uint8_t    findDateForWeek(uint16_t year, uint8_t month, uint8_t dow, int8_t nthWeek);
};
#endif /* TIMEUTILS_H_ */
