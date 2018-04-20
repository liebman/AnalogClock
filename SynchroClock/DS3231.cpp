/*
 * DS3231.cpp
 *
 * Copyright 2017 Christopher B. Liebman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 *  Created on: May 21, 2017
 *      Author: chris.l
 */

#include "DS3231.h"

static PROGMEM const char TAG[] = "DS3231";

DS3231::DS3231()
{
}

int DS3231::begin()
{
    dlog.info(FPSTR(TAG),F("::begin: enable OSC/no BBS/no CONV/1hz SQWV on/no ALRM"));

    uint8_t ctrl = 0b00000000;       // enable osc/no BBS/no CONV/1hz/SQWV on/no ALRM
    int err = write(DS3231_CONTROL_REG, ctrl); //CONTROL Register Address
    if (err)
    {
        dlog.error(FPSTR(TAG), F("::begin: write(DS3231_CONTROL_REG) failed: %d"), err);
        return -1;
    }

    dlog.info(FPSTR(TAG),F("::begin: small delay"));
    delay(100);

    dlog.info(FPSTR(TAG),F("::begin: reading HOUR register to insure 24hr format"));
    // set the clock to 24hr format
    uint8_t hr;
    if (read(DS3231_HOUR_REG, &hr))
    {
        dlog.error(FPSTR(TAG), F("::begin(): read(DS3231_HOUR_REG) failed!"));
        return -1;
    }

     // switch to 24hr mode if its not already
    if (hr & _BV(DS3231_AMPM))
    {
        dlog.info(FPSTR(TAG),F("::begin: writing HOUR register to set 24hr format"));

        hr &= ~_BV(DS3231_AMPM);

        if(write(DS3231_HOUR_REG, hr))
        {
            dlog.error(FPSTR(TAG), F("::begin(): write(DS3231_HOUR_REG, 0x%02x) failed!"), hr);
            return -1;
        }
    }

    dlog.info(FPSTR(TAG), F("::begin: done"));
    return 0;
}

uint8_t DS3231::fromBCD(uint8_t val)
{
	return val - 6 * (val >> 4);
}

uint8_t DS3231::toBCD(uint8_t val)
{
	return val + 6 * (val / 10);
}

int DS3231::readTime(DS3231DateTime& dt)
{

	uint8_t count = setupRead(DS3231_SEC_REG, 7);
	if (count != 7)
	{
		dlog.error(FPSTR(TAG), F("::readTime: setupRead failed! count:%u != 7"), count);
		Wire.clearWriteError();
		Wire.flush();
		return -1;
	}

	uint8_t seconds = Wire.read();
	uint8_t minutes = Wire.read();
	uint8_t hours   = Wire.read();
	uint8_t day     = Wire.read();
	uint8_t date    = Wire.read();
	uint8_t month   = Wire.read();
	uint8_t year    = Wire.read();

	dlog.trace(FPSTR(TAG), F("::readTime  raw month: 0x%02x"), month);

    dt.seconds  = fromBCD(seconds);
    dt.minutes  = fromBCD(minutes);
    dt.hours    = fromBCD(hours & DS3231_24HR_MASK);
    dt.day      = fromBCD(day);
    dt.date     = fromBCD(date);
    dt.month    = fromBCD(month & DS3231_MONTH_MASK);
    dt.year     = fromBCD(year);
    dt.century  = month&DS3231_CENTURY? 1 : 0;

    if (!dt.isValid())
    {
        dlog.error(FPSTR(TAG), F("::readTime: result not valid!!!"));
        Wire.clearWriteError();
        Wire.flush();
        return -1;
    }

    dlog.debug(FPSTR(TAG), F("::readTime %04u-%02u-%02u %02u:%02u:%02u day:%u century:%u"),
             dt.year,
             dt.month,
             dt.date,
             dt.hours,
             dt.minutes,
             dt.seconds,
             dt.day,
             dt.century);
	return 0;
}

int DS3231::writeTime(DS3231DateTime &dt)
{
    dlog.debug(FPSTR(TAG), F("::writeTime %04u-%02u-%02u %02u:%02u:%02u day:%u century:%u"),
             dt.year,
             dt.month,
             dt.date,
             dt.hours,
             dt.minutes,
             dt.seconds,
             dt.day,
             dt.century);

    Wire.beginTransmission(DS3231_ADDRESS);
    if (Wire.write(DS3231_SEC_REG) != 1)
    {
        Wire.endTransmission();
        dlog.error(FPSTR(TAG), F("::writeTime: Wire.write(reg=DS3231_SEC_REG) failed!"));
        return -1;
    }

    int count;
    count  = Wire.write(toBCD(dt.seconds));
    count += Wire.write(toBCD(dt.minutes));
    count += Wire.write(toBCD(dt.hours));
    count += Wire.write(toBCD(dt.day));
    count += Wire.write(toBCD(dt.date));
    count += Wire.write(toBCD(dt.month) | (dt.century ? _BV(DS3231_CENTURY) : 0));
    count += Wire.write(toBCD(dt.year));
    if (count != 7)
    {
        dlog.error(FPSTR(TAG), F("::writeTime: Wire.write() all fields, expected 7 got %u"), count);
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dlog.error(FPSTR(TAG), F("::writeTime: Wire.endTransmission() returned: %d"), err);
        return -1;
    }
    return(0);
}

int DS3231::setupRead(uint8_t reg, uint8_t size)
{
    Wire.beginTransmission(DS3231_ADDRESS);
    if (Wire.write(reg) != 1)
    {
        Wire.endTransmission();
        dlog.error(FPSTR(TAG), F("::setupRead: Wire.write(DS3231_CONTROL_REG) failed!"));
        return -1;
    }
    int err = Wire.endTransmission();

    if (err)
    {
        dlog.error(FPSTR(TAG), F("::setupRead: Wire.endTransmission() returned: %d"), err);
        return -1;
    }

    return Wire.requestFrom(DS3231_ADDRESS, (size_t)size);
}

int DS3231::read(uint8_t reg, uint8_t *value)
{
    size_t count = setupRead(reg, 1);
    count = Wire.requestFrom(DS3231_ADDRESS, (size_t)1);
    *value = Wire.read();
    if (count != 1)
    {
        dlog.error(FPSTR(TAG), F("::read: Wire.requestFrom() returns %u, expected 1"), count);
        return -1;
    }
    return 0;
}


int DS3231::write(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(DS3231_ADDRESS);
    if (Wire.write(reg) != 1)
    {
        Wire.endTransmission();
        dlog.error(FPSTR(TAG), F("::write: Wire.write(reg=%d, value=%d) failed!"), reg, value);
        return -1;
    }
    size_t count;
    count = Wire.write(value);
    if (count != 1)
    {
        dlog.error(FPSTR(TAG), F("::write: Wire.write(value=%u) returns %u"), value, count);
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dlog.error(FPSTR(TAG), F("::write: Wire.endTransmission() returned: %d"), err);
        return -1;
    }
    return(0);
}
