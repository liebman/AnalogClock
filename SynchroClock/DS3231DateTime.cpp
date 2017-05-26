/*
 * DS3231DateTime.cpp
 *
 *  Created on: May 22, 2017
 *      Author: chris.l
 */

#include "DS3231DateTime.h"


#ifdef DEBUG_DS3231_DATE_TIME
unsigned int snprintf(char*, unsigned int, ...);
#define DBP_BUF_SIZE 256
#define dbbegin(x)     Serial.begin(x);
#define dbprintf(...)  {char dbp_buf[DBP_BUF_SIZE]; snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprint(x)     Serial.print(x)
#define dbprintln(x)   Serial.println(x)
#define dbflush()      Serial.flush()
#define dbvalue(x)     debugPrint(x)
#else
#define dbbegin(x)
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#define dbflush()
#define dbvalue(x)
#endif

DS3231DateTime::DS3231DateTime()
{
    seconds = 0;
    minutes = 0;
    hours = 0;
    date = 0;
    day = 0;
    month = 0;
    year = 0;
    century = 0;
}

void DS3231DateTime::setUnixTime(unsigned long time)
{
    struct tm tm;
    gmtime_r((time_t*)&time, &tm);

    dbprintf("DS3231DateTime::setUnixTime month: %d\n", tm.tm_mon);

    seconds = tm.tm_sec;
    minutes = tm.tm_min;
    hours   = tm.tm_hour;
    day     = tm.tm_wday;
    date    = tm.tm_mday;
    month   = tm.tm_mon  + 1;
    year    = tm.tm_year - 100;
    century = 0;
    dbvalue("setUnixTime new value:");
}

unsigned long DS3231DateTime::getUnixTime()
{
    struct tm tm;
    tm.tm_sec   = seconds;
    tm.tm_min   = minutes;
    tm.tm_hour  = hours;
    tm.tm_mday  = date;
    tm.tm_mon   = month - 1;
    tm.tm_year  = year  + 100;
    tm.tm_isdst = 0;
    tm.tm_wday  = 0;
    tm.tm_yday  = 0;
    unsigned long unix = mktime(&tm);
    dbprintf("returning unix time: %lu\n", unix);
    return unix;
}

uint16_t DS3231DateTime::getPosition()
{
    int h = hours;
    if (h > 12)
    {
        h -= 12;
    }
    return h*3600 + minutes*60 + seconds;
}

uint16_t DS3231DateTime::getPosition(int offset)
{
    int signed_position = getPosition();
    dbprintf("position before offset: %d\n", signed_position);
    signed_position += offset;
    dbprintf("position after offset: %d\n", signed_position);
    if (signed_position < 0)
    {
        signed_position += MAX_POSITION;
        dbprintf("position corrected+: %d\n", signed_position);
    }
    else if (signed_position >= MAX_POSITION)
    {
        signed_position -= MAX_POSITION;
        dbprintf("position corrected-: %d\n", signed_position);
    }
    uint16_t position = (uint16_t) signed_position;
    return position;
}

char value_buf[128];

const char* DS3231DateTime::string()
{
    snprintf(value_buf, 127,
            "%04u-%02u-%02u %02u:%02u:%02u",
            year+1900+100,
            month,
            date,
            hours,
            minutes,
            seconds);
    return value_buf;
}

#ifdef DEBUG_DS3231_DATE_TIME
void DS3231DateTime::debugPrint(const char* prefix)
{
    uint16_t      position = getPosition();
    unsigned long unix     = getUnixTime();

    dbprintf("%s position:%u (%04u-%02u-%02u %02u:%02u:%02u) weekday:%u century:%d unix:%lu\n",
            prefix,
            position,
            year+1900+100,
            month,
            date,
            hours,
            minutes,
            seconds,
            day,
            century,
            unix);
}
#endif

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
time_t DS3231DateTime::mktime(struct tm *tmbuf)
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

struct tm *DS3231DateTime::gmtime_r(const time_t *timer, struct tm *tmbuf)
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
