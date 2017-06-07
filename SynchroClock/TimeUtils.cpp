/*
 * TimeUtils.cpp
 *
 *  Created on: Jun 7, 2017
 *      Author: liebman
 */

#include "TimeUtils.h"

#ifdef DEBUG_TIME_UTILS
#define DBP_BUF_SIZE 256
#define dbprintf(...) logger.printf(__VA_ARGS__)
#define dbprintln(x)  logger.println(x)
#define dbflush()     logger.flush()
#else
#define dbprintf(...)
#define dbprintln(x)
#define dbflush()
#endif

uint8_t TimeUtils::parseSmallDuration(const char* value)
{
    int i = atoi(value);
    if (i < 0 || i > 255)
    {
        dbprintf("invalid value for %s: using 32 instead!\n", value);
        i = 32;
    }
    return (uint8_t) i;
}

int TimeUtils::parseOffset(const char* offset_string)
{
    int result = 0;
    char value[11];
    strncpy(value, offset_string, 10);
    if (strchr(value, ':') != NULL)
    {
        int sign = 1;
        char* s;

        if (value[0] == '-')
        {
            sign = -1;
            s = strtok(&(value[1]), ":");
        }
        else
        {
            s = strtok(value, ":");
        }
        if (s != NULL)
        {
            int h = atoi(s);
            while (h > 11)
            {
                h -= 12;
            }

            result += h * 3600; // hours to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s) * 60; // minutes to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s);
        }
        // apply sign
        result *= sign;
    }
    else
    {
        result = atoi(value);
        if (result < -43199 || result > 43199)
        {
            result = 0;
        }
    }
    return result;
}

uint16_t TimeUtils::parsePosition(const char* position_string)
{
    int result = 0;
    char value[10];
    strncpy(value, position_string, 9);
    if (strchr(value, ':') != NULL)
    {
        char* s = strtok(value, ":");

        if (s != NULL)
        {
            int h = atoi(s);
            while (h > 11)
            {
                h -= 12;
            }

            result += h * 3600; // hours to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s) * 60; // minutes to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s);
        }
    }
    else
    {
        result = atoi(value);
        if (result < 0 || result > 43199)
        {
            result = 0;
        }
    }
    return result;
}


//
// Modified code from: http://www.jbox.dk/sanos/source/lib/time.c.html
//

#define YEAR0                   1900
#define EPOCH_YR                1970
#define SECS_DAY                (24L * 60L * 60L)
#define LEAPYEAR(year)          (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEARSIZE(year)          (LEAPYEAR(year) ? 366 : 365)
#define TIME_MAX                2147483647L

const int _ytab[2][12] =
{
{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };

// esp version is broken :-(
time_t TimeUtils::mktime(struct tm *tmbuf)
{
    long day, year;
    int tm_year;
    int yday, month;
    /*unsigned*/long seconds;
    int overflow;

    tmbuf->tm_min += tmbuf->tm_sec / 60;
    tmbuf->tm_sec %= 60;
    if (tmbuf->tm_sec < 0)
    {
        tmbuf->tm_sec += 60;
        tmbuf->tm_min--;
    }
    tmbuf->tm_hour += tmbuf->tm_min / 60;
    tmbuf->tm_min = tmbuf->tm_min % 60;
    if (tmbuf->tm_min < 0)
    {
        tmbuf->tm_min += 60;
        tmbuf->tm_hour--;
    }
    day = tmbuf->tm_hour / 24;
    tmbuf->tm_hour = tmbuf->tm_hour % 24;
    if (tmbuf->tm_hour < 0)
    {
        tmbuf->tm_hour += 24;
        day--;
    }
    tmbuf->tm_year += tmbuf->tm_mon / 12;
    tmbuf->tm_mon %= 12;
    if (tmbuf->tm_mon < 0)
    {
        tmbuf->tm_mon += 12;
        tmbuf->tm_year--;
    }
    day += (tmbuf->tm_mday - 1);
    while (day < 0)
    {
        if (--tmbuf->tm_mon < 0)
        {
            tmbuf->tm_year--;
            tmbuf->tm_mon = 11;
        }
        day += _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
    }
    while (day >= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon])
    {
        day -= _ytab[LEAPYEAR(YEAR0 + tmbuf->tm_year)][tmbuf->tm_mon];
        if (++(tmbuf->tm_mon) == 12)
        {
            tmbuf->tm_mon = 0;
            tmbuf->tm_year++;
        }
    }
    tmbuf->tm_mday = day + 1;
    year = EPOCH_YR;
    if (tmbuf->tm_year < year - YEAR0)
        return (time_t) -1;
    seconds = 0;
    day = 0;                      // Means days since day 0 now
    overflow = 0;

    // Assume that when day becomes negative, there will certainly
    // be overflow on seconds.
    // The check for overflow needs not to be done for leapyears
    // divisible by 400.
    // The code only works when year (1970) is not a leapyear.
    tm_year = tmbuf->tm_year + YEAR0;

    if (TIME_MAX / 365 < tm_year - year)
        overflow++;
    day = (tm_year - year) * 365;
    if (TIME_MAX - day < (tm_year - year) / 4 + 1)
        overflow++;
    day += (tm_year - year) / 4 + ((tm_year % 4) && tm_year % 4 < year % 4);
    day -= (tm_year - year) / 100
            + ((tm_year % 100) && tm_year % 100 < year % 100);
    day += (tm_year - year) / 400
            + ((tm_year % 400) && tm_year % 400 < year % 400);

    yday = month = 0;
    while (month < tmbuf->tm_mon)
    {
        yday += _ytab[LEAPYEAR(tm_year)][month];
        month++;
    }
    yday += (tmbuf->tm_mday - 1);
    if (day + yday < 0)
        overflow++;
    day += yday;

    tmbuf->tm_yday = yday;
    tmbuf->tm_wday = (day + 4) % 7;               // Day 0 was thursday (4)

    seconds = ((tmbuf->tm_hour * 60L) + tmbuf->tm_min) * 60L + tmbuf->tm_sec;

    if ((TIME_MAX - seconds) / SECS_DAY < day)
        overflow++;
    seconds += day * SECS_DAY;

    if (overflow)
        return (time_t) -1;

    if ((time_t) seconds != seconds)
        return (time_t) -1;
    return (time_t) seconds;
}

struct tm *TimeUtils::gmtime_r(const time_t *timer, struct tm *tmbuf)
{
  time_t time = *timer;
  unsigned long dayclock, dayno;
  int year = EPOCH_YR;

  dayclock = (unsigned long) time % SECS_DAY;
  dayno = (unsigned long) time / SECS_DAY;

  tmbuf->tm_sec = dayclock % 60;
  tmbuf->tm_min = (dayclock % 3600) / 60;
  tmbuf->tm_hour = dayclock / 3600;
  tmbuf->tm_wday = (dayno + 4) % 7; // Day 0 was a thursday
  while (dayno >= (unsigned long) YEARSIZE(year)) {
    dayno -= YEARSIZE(year);
    year++;
  }
  tmbuf->tm_year = year - YEAR0;
  tmbuf->tm_yday = dayno;
  tmbuf->tm_mon = 0;
  while (dayno >= (unsigned long) _ytab[LEAPYEAR(year)][tmbuf->tm_mon]) {
    dayno -= _ytab[LEAPYEAR(year)][tmbuf->tm_mon];
    tmbuf->tm_mon++;
  }
  tmbuf->tm_mday = dayno + 1;
  tmbuf->tm_isdst = 0;
  return tmbuf;
}


/*--------------------------------------------------------------------------
  FUNC: 6/11/11 - Returns day of week for any given date
  PARAMS: year, month, date
  RETURNS: day of week (0-7 is Sun-Sat)
  NOTES: Sakamoto's Algorithm
    http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week#Sakamoto.27s_methods
    Altered to use char when possible to save microcontroller ram
--------------------------------------------------------------------------*/
uint8_t TimeUtils::findDOW(uint16_t y, uint8_t m, uint8_t d)
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
uint8_t TimeUtils::findNthDate(uint16_t year, uint8_t month, uint8_t dow, uint8_t nthWeek)
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

uint8_t TimeUtils::daysInMonth(uint16_t year, uint8_t month)
{
    uint8_t days = 31;

    switch(month)
    {
     case 2:
         days = 28;
         if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
         {
             days = 29;
         }
         break;
     case 4:
     case 6:
     case 9:
     case 11:
            days = 30;
            break;
    }
    return days;
}

uint8_t TimeUtils::findDateForWeek(uint16_t year, uint8_t month, uint8_t dow, int8_t week)
{
    uint8_t weeks[5];
    uint8_t max_day = daysInMonth(year, month);
    int last = 0;

    if (week >= 0)
    {
        return findNthDate(year, month, dow, week);
    }

    //
    // find all times this weekday shows up in the month
    // Note that 'last' will end up pointing 1 past the last
    // valid occurrence.  -1 will give the last one.
    //
    for(last = 0; last <= 5; ++last)
    {
        weeks[last] = findNthDate(year, month, dow, last);
        if (weeks[last] > max_day)
        {
            break;
        }
    }

    return weeks[last+week];
}

int TimeUtils::computeUTCOffset(uint16_t year, uint8_t month, uint8_t mday, uint8_t hour, TimeChange* tc, int tc_count)
{
    //
    // TODO: need to map dt into current timezone!
    //
    int offset = 0;
    for (int i = 0; i < tc_count;++i)
    {
        dbprintf("TimeUtils::computeUTCOffset: index:%d offset:%d month:%u dow:%u occurrence:%u hour:%u\n",
                i,
                tc[i].tz_offset,
                tc[i].month,
                tc[i].day_of_week,
                tc[i].occurrence,
                tc[i].hour);

        dbprintf("TimeUtils::computeUTCOffset: month is %u\n", month);

        // if we are past this month then we can use this offset.
        if (month > tc[i].month)
        {
            offset = tc[i].tz_offset;
            dbprintf("TimeUtils::computeUTCOffset: after month, offset: %d\n", offset);
            continue;
        }

        // if this is not then month yet then we can't use it.
        if (month != tc[i].month)
        {
            dbprintln("TimeUtils::computeUTCOffset: month does not match, skipping entry");
            continue;
        }

        // compute what date the occurrence of day_of_week is.
        uint8_t date = findDateForWeek(year, tc[i].month, tc[i].day_of_week, tc[i].occurrence);
        dbprintf("TimeUtils::computeUTCOffset: %u is the one we are looking for!\n", date);
        dbprintf("TimeUtils::computeUTCOffset: date is %u\n", mday);

        // if we are past this date we can use the offset
        if (mday > date)
        {
            offset = tc[i].tz_offset;
            dbprintf("TimeUtils::computeUTCOffset: after date, offset: %d\n", offset);
            continue;
        }

        // if this is not then date yet then we can't use it.
        if (mday != date)
        {
            dbprintln("TimeUtils::computeUTCOffset: date does not match, skipping entry");
            continue;
        }

        dbprintf("TimeUtils::computeUTCOffset: hour is %u\n", hour);

        // if we are past this month then we can use this offset.
        if (hour > tc[i].hour)
        {
            offset = tc[i].tz_offset;
            dbprintf("TimeUtils::computeUTCOffset: after hour, offset: %d\n", offset);
            continue;
        }

        // if this is not then month yet then we can't use it.
        if (hour != tc[i].hour)
        {
            dbprintln("TimeUtils::computeUTCOffset: hour does not match, skipping entry");
            continue;
        }

        dbprintf("TimeUtils::computeUTCOffset: its all a match, offset: %d\n", offset);
        offset = tc[i].tz_offset;
    }
    return offset;
}