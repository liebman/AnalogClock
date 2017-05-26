/*
 * Clock.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: liebman
 */

#include "Clock.h"

#ifdef DEBUG_CLOCK
unsigned int snprintf(char*, unsigned int, ...);
#define DBP_BUF_SIZE 256

#define dbbegin(x)     Serial.begin(x);
#define dbprintf(...)  {char dbp_buf[DBP_BUF_SIZE]; snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprint(x)     Serial.print(x)
#define dbprintln(x)   Serial.println(x)
#define dbflush()      Serial.flush()
#else
#define dbbegin(x)
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#define dbflush()
#endif

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
    waitForActive();
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
	return write(CMD_ADJUSTMENT, value);
}

int Clock::readPosition(uint16_t *value)
{
	return read(CMD_POSITION, value);
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

int Clock::readAPDuration(uint8_t* value)
{
    return read(CMD_AP_DURATION, value);
}

int Clock::writeAPDuration(uint8_t value)
{
    return write(CMD_AP_DURATION, value);
}

int Clock::readAPDelay(uint8_t* value)
{
    return read(CMD_AP_DELAY, value);
}

int Clock::writeAPDelay(uint8_t value)
{
    return write(CMD_AP_DELAY, value);
}

int Clock::readSleepDelay(uint8_t* value)
{
    return read(CMD_SLEEP_DELAY, value);
}

int Clock::writeSleepDelay(uint8_t value)
{
    return write(CMD_SLEEP_DELAY, value);
}

bool Clock::getEnable()
{
    return getCommandBit(BIT_ENABLE);
}

void Clock::setEnable(bool enable)
{
    setCommandBit(enable, BIT_ENABLE);
}

bool Clock::getStayActive()
{
    return getCommandBit(BIT_STAY_ACTIVE);
}

void Clock::setStayActive(bool active)
{
#ifndef DISABLE_ACTIVE
    waitForActive();
    setCommandBit(active, BIT_STAY_ACTIVE);
#endif
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

void Clock::setCommandBit(bool onoff, uint8_t bit)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    int err = Wire.endTransmission();
    if (err != 0)
    {
        dbprintf("Clock::setCommandBit endTransmission returned: %d\n", err);
    }

    Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    uint8_t value = Wire.read();

    if (onoff)
    {
        value |= bit;
    }
    else
    {
        value &= ~bit;
    }


    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    Wire.write(value);
    Wire.endTransmission();
}

// send a command, read a 16 bit value
int Clock::read16(uint8_t command, uint16_t*value)
{
    return read(command, (uint8_t*)value, sizeof(uint16_t));
}

// send a command, read a 16 bit value
uint16_t Clock::read16(uint8_t command)
{
    uint16_t value;
    read(command, (uint8_t*)&value, sizeof(value));
    return (value);
}

// send a command with a 16 bit value
int Clock::write16(uint8_t command, uint16_t value)
{
    return write(command, (uint8_t*)&value, sizeof(uint16_t));
}

// send a command, read a 8 bit value
int Clock::read8(uint8_t command, uint8_t* value)
{
    return read(command, value, sizeof(uint8_t));
}

// send a command, read a 8 bit value
uint8_t Clock::read8(uint8_t command)
{
    uint8_t value;
    read(command, &value, sizeof(value));
    return (value);
}

// send a command with a 8 bit value
int Clock::write8(uint8_t command, uint8_t value)
{
    return write(command, &value, sizeof(value));
}


int Clock::read(uint8_t command, uint8_t *value, size_t size)
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
    count = Wire.requestFrom(I2C_ADDRESS, size);
    Wire.readBytes(value, size);
    if (count != size)
    {
        dbprintf("Clock::read: Wire.requestFrom() returns %u, expected %u\n", count, size);
        return -1;
    }
    return 0;
}

//
// write command & size bytes to clock, returns bytes writen & -1 on error.
//
int Clock::write(uint8_t command, uint8_t *value, size_t size)
{
    Wire.beginTransmission(I2C_ADDRESS);
    if (Wire.write(command) != 1)
    {
        Wire.endTransmission();
        dbprintf("Clock::write: Wire.write(command=%d) failed!", command);
        return -1;
    }
    size_t count;
    count = Wire.write(value, size);
    if (count != size)
    {
        dbprintf("Clock::write: Wire.write() returns %u, expected %u\n", count, size);
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

int Clock::read(uint8_t command, uint8_t *value)
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

void Clock::waitForActive()
{
    waitForEdge(CLOCK_EDGE_FALLING);
    delay(25); // give the chip time to wake up!
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
