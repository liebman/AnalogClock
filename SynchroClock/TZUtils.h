/*
 * TZUtils.h
 *
 *  Created on: Jun 5, 2017
 *      Author: chris.l
 */

#ifndef TZUTILS_H_
#define TZUTILS_H_
#include "DS3231DateTime.h"
#include "Logger.h"

#define DEBUG_TZ_UTILS

typedef struct
{
    int     tz_offset;     // seconds offset from UTC
    uint8_t month;         // starting month that this takes effect.
    uint8_t occurrence;     // 2 = second occurrence of the given day.
    uint8_t day_of_week;   // 0 = Sunday
    uint8_t hour;
} TZInfo;


class TZUtils
{
public:
    static int computeCurrentTZOffset(DS3231DateTime &dt, TZInfo* tz, int tz_count);

private:
    static uint8_t findDOW(uint16_t y, uint8_t m, uint8_t d);
    static uint8_t findNthDate(uint16_t year, uint8_t month, uint8_t dow, uint8_t nthWeek);
};

#endif /* TZUTILS_H_ */
