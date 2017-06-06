/*
 * TZUtils.cpp
 *
 *  Created on: Jun 5, 2017
 *      Author: chris.l
 */

#include "TZUtils.h"

#ifdef DEBUG_TZ_UTILS
#define DBP_BUF_SIZE 256
#define dbprintf(...) logger.printf(__VA_ARGS__)
#define dbprintln(x)  logger.println(x)
#define dbflush()     logger.flush()
#else
#define dbprintf(...)
#define dbprintln(x)
#define dbflush()
#endif

/*--------------------------------------------------------------------------
  FUNC: 6/11/11 - Returns day of week for any given date
  PARAMS: year, month, date
  RETURNS: day of week (0-7 is Sun-Sat)
  NOTES: Sakamoto's Algorithm
    http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week#Sakamoto.27s_methods
    Altered to use char when possible to save microcontroller ram
--------------------------------------------------------------------------*/
uint8_t TZUtils::findDOW(uint16_t y, uint8_t m, uint8_t d)
{
    static char t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

/*--------------------------------------------------------------------------
  FUNC: 6/11/11 - Returns the date for Nth day of month. For instance,
    it will return the numeric date for the 2nd Sunday of April
  PARAMS: year, month, day of week, Nth occurrence of that day in that month
  RETURNS: date
  NOTES: There is no error checking for invalid inputs.
--------------------------------------------------------------------------*/
uint8_t TZUtils::findNthDate(uint16_t year, uint8_t month, uint8_t dow, uint8_t nthWeek)
{
    uint8_t targetDate = 1;
    uint8_t firstDOW = findDOW(year,month,targetDate);
    while (firstDOW != dow) {
        firstDOW = (firstDOW+1)%7;
        targetDate++;
    }
    //Adjust for weeks
    targetDate += (nthWeek-1)*7;
    return targetDate;
}

int TZUtils::computeCurrentTZOffset(DS3231DateTime &dt, TZInfo* tz, int tz_count)
{
    //
    // TODO: need to map dt into current timezone!
    //
    int offset = 0;
    for (int i = 0; i < tz_count;++i)
    {
        dbprintf("TZUtils::computeCurrentTZOffset: index:%d offset:%d month:%u dow:%u occurrence:%u hour:%u\n",
                i,
                tz[i].tz_offset,
                tz[i].month,
                tz[i].day_of_week,
                tz[i].occurrence,
                tz[i].hour);

        dbprintf("TZUtils::computeCurrentTZOffset: month is %u\n", dt.getMonth());

        // if we are past this month then we can use this offset.
        if (dt.getMonth() > tz[i].month)
        {
            offset = tz[i].tz_offset;
            dbprintf("TZUtils::computeCurrentTZOffset: after month, offset: %d\n", offset);
            continue;
        }

        // if this is not then month yet then we can't use it.
        if (dt.getMonth() != tz[i].month)
        {
            dbprintln("TZUtils::computeCurrentTZOffset: month does not match, skipping entry");
            continue;
        }

        // compute what date the occurrence of day_of_week is.
        uint8_t date = findNthDate(dt.getYear(), tz[i].month, tz[i].day_of_week, tz[i].occurrence);
        dbprintf("TZUtils::computeCurrentTZOffset: %u is the one we are looking for!\n", date);
        dbprintf("TZUtils::computeCurrentTZOffset: date is %u\n", dt.getDate());

        // if we are past this date we can use the offset
        if (dt.getDate() > date)
        {
            offset = tz[i].tz_offset;
            dbprintf("TZUtils::computeCurrentTZOffset: after date, offset: %d\n", offset);
            continue;
        }

        // if this is not then date yet then we can't use it.
        if (dt.getDate() != date)
        {
            dbprintln("TZUtils::computeCurrentTZOffset: date does not match, skipping entry");
            continue;
        }

        dbprintf("TZUtils::computeCurrentTZOffset: hour is %u\n", dt.getHour());

        // if we are past this month then we can use this offset.
        if (dt.getHour() > tz[i].hour)
        {
            offset = tz[i].tz_offset;
            dbprintf("TZUtils::computeCurrentTZOffset: after hour, offset: %d\n", offset);
            continue;
        }

        // if this is not then month yet then we can't use it.
        if (dt.getHour() != tz[i].hour)
        {
            dbprintln("TZUtils::computeCurrentTZOffset: hour does not match, skipping entry");
            continue;
        }

        dbprintf("TZUtils::computeCurrentTZOffset: its all a match, offset: %d\n", offset);
        offset = tz[i].tz_offset;
    }
    return offset;
}
