/*
 * DS3231DateTime.h
 *
 *  Created on: May 22, 2017
 *      Author: chris.l
 */

#ifndef DS3231DATETIME_H_
#define DS3231DATETIME_H_
#include <Arduino.h>
#include <time.h>
#include "Logger.h"

#define DEBUG_DS3231_DATE_TIME

#define MAX_POSITION      43200 // seconds in 12 hours

class DS3231DateTime
{
public:
	DS3231DateTime();
	boolean       isValid();
	void          setUnixTime(unsigned long time);
	unsigned long getUnixTime();
	uint16_t      getPosition();
    uint16_t      getPosition(int offset);
    const char*   string();
#ifdef DEBUG_DS3231_DATE_TIME
	void          debugPrint(const char* prefix);
#endif

	friend class DS3231;
protected:
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t date;
	uint8_t day;
	uint8_t month;
	uint8_t year;
    uint8_t century;

private:
    time_t mktime(struct tm *tmbuf);
    struct tm *gmtime_r(const time_t *timer, struct tm *tmbuf);
};

#endif /* DS3231DATETIME_H_ */
