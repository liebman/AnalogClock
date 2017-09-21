/*
 * Clock.cpp
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
 *  Created on: Mar 26, 2017
 *      Author: liebman
 */

#include "Clock.h"

#define DEBUG
#include "Logger.h"

Clock::Clock(int _pin)
{
    pin = _pin;
}

int Clock::begin()
{
	if (isClockPresent())
	{
		return 0;
	}
	return -1;
}

//
// access to analog clock controller via i2c
//

bool Clock::isClockPresent()
{
    uint8_t id;
    if (read(CMD_ID, &id))
    {
    	return false;
    }
    return (id == CLOCK_ID_VALUE);
}

int Clock::readAdjustment(uint16_t *value)
{
	return read(CMD_ADJUSTMENT, value);
}

int Clock::writeAdjustment(uint16_t value)
{
	if (value >= CLOCK_MAX)
	{
		dbprintf("Clock::writeAdjustment: invalid value: %u", value);
		return -1;
	}
	return write(CMD_ADJUSTMENT, value);
}

int Clock::readPosition(uint16_t *value)
{
	int err = read(CMD_POSITION, value);
	if (err)
	{
		return err;
	}
	if (*value >= CLOCK_MAX)
	{
		dbprintf("Clock::readPosition: INVALID VALUE RETURNED: %u\n", *value);
		return -1;
	}
	return 0;
}

int Clock::writePosition(uint16_t value)
{
	return write(CMD_POSITION, value);
}

int Clock::readTPDuration(uint8_t *value)
{
    return read(CMD_TP_DURATION, value);
}

int Clock::writeTPDuration(uint8_t value)
{
    return write(CMD_TP_DURATION, value);
}

int Clock::readTPDuty(uint8_t *value)
{
    return read(CMD_TP_DUTY, value);
}

int Clock::writeTPDuty(uint8_t value)
{
    return write(CMD_TP_DUTY, value);
}

int Clock::readAPDuration(uint8_t* value)
{
    return read(CMD_AP_DURATION, value);
}

int Clock::writeAPDuration(uint8_t value)
{
    return write(CMD_AP_DURATION, value);
}

int Clock::readAPDuty(uint8_t *value)
{
    return read(CMD_AP_DUTY, value);
}

int Clock::writeAPDuty(uint8_t value)
{
    return write(CMD_AP_DUTY, value);
}

int Clock::readAPDelay(uint8_t* value)
{
    return read(CMD_AP_DELAY, value);
}

int Clock::writeAPDelay(uint8_t value)
{
    return write(CMD_AP_DELAY, value);
}

bool Clock::getEnable()
{
    return getCommandBit(BIT_ENABLE);
}

void Clock::setEnable(bool enable)
{
    setCommandBit(enable, BIT_ENABLE);
}


bool Clock::getCommandBit(uint8_t bit)
{
    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    int err = Wire.endTransmission(true);
    if (err != 0)
    {
        dbprintf("Clock::getCommandBit endTransmission returned: %d\n", err);
    }

    int size = Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    if (size != 1)
    {
        dbprintf("Clock::getCommandBit requestFrom did not return 1, size:%u\n", size);
    }
    uint8_t value = Wire.read();
    return ((value & bit) == bit);
}

int Clock::setCommandBit(bool onoff, uint8_t bit)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    int err = Wire.endTransmission();
    if (err != 0)
    {
        dbprintf("Clock::setCommandBit endTransmission returned: %d\n", err);
        return -1;
    }

    size_t count = Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    uint8_t value = Wire.read();

    if (count != 1)
    {
    	dbprintln("Clock::setCommandBit: Wire.requestFrom failed!");
    	return -1;
    }

    if (onoff)
    {
        value |= bit;
    }
    else
    {
        value &= ~bit;
    }


    Wire.beginTransmission(I2C_ADDRESS);
    count = 0;
    count += Wire.write(CMD_CONTROL);
    count += Wire.write(value);
    if (count != 2)
    {
        Wire.endTransmission();
    	dbprintln("Clock::setCommandBit: Wire.write command & value failed!");
    	return -1;
    }
    err = Wire.endTransmission();
    if (err)
    {
        dbprintf("Clock::setCommandBit: Wire.endTransmission() returned: %d\n", err);
        return -1;
    }

    return 0;
}

int Clock::read(uint8_t command, uint8_t *value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    if (Wire.write(command) != 1)
    {
        Wire.endTransmission();
        dbprintln("Clock::read: Wire.write(command) failed!");
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dbprintf("Clock::read: Wire.endTransmission() returned: %d\n", err);
        return -1;
    }
    size_t count;
    count = Wire.requestFrom(I2C_ADDRESS, 1);
    *value = Wire.read();
    if (count != 1)
    {
        dbprintf("Clock::read: Wire.requestFrom() returns %u, expected 1\n", count);
        return -1;
    }
    return 0;
}

int Clock::write(uint8_t command, uint8_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    if (Wire.write(command) != 1)
    {
        Wire.endTransmission();
        dbprintf("Clock::write: Wire.write(command=%d) failed!", command);
        return -1;
    }
    size_t count;
    count = Wire.write(value);
    if (count != 1)
    {
        dbprintf("Clock::write: Wire.write() returns %u, expected 1\n", count);
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dbprintf("Clock::read: Wire.endTransmission() returned: %d\n", err);
        return -1;
    }
    return 0;
}

int Clock::read(uint8_t command, uint16_t *value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    if (Wire.write(command) != 1)
    {
        Wire.endTransmission();
        dbprintln("Clock::read: Wire.write(I2C_ADDRESS) failed!");
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dbprintf("Clock::read: Wire.endTransmission() returned: %d\n", err);
        return -1;
    }
    size_t count;
    count = Wire.requestFrom(I2C_ADDRESS, 2);
    *value = (Wire.read() & 0xff) | (Wire.read() & 0xff) << 8;
    if (count != 2)
    {
        dbprintf("Clock::read: Wire.requestFrom() returns %u, expected 2\n", count);
        return -1;
    }
    return 0;
}

int Clock::write(uint8_t command, uint16_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    if (Wire.write(command) != 1)
    {
        Wire.endTransmission();
        dbprintf("Clock::write: Wire.write(command=%d) failed!", command);
        return -1;
    }
    size_t count = 0;
    count += Wire.write(value & 0xff);
    count += Wire.write(value >> 8);
    if (count != 2)
    {
        dbprintf("Clock::write: Wire.write() returns %u, expected 2\n", count);
        return -1;
    }
    int err = Wire.endTransmission();
    if (err)
    {
        dbprintf("Clock::read: Wire.endTransmission() returned: %d\n", err);
        return -1;
    }
    return 0;
}

void Clock::waitForEdge(int edge)
{
    while (digitalRead(pin) == edge)
    {
        delay(1);
    }
    while (digitalRead(pin) != edge)
    {
        delay(1);
    }
}
