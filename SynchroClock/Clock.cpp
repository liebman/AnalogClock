/*
 * Clock.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: liebman
 */

#include "Clock.h"

Clock::Clock()
{
}

//
// access to analog clock controler via i2c
//
uint16_t Clock::getClockAdjustment()
{
    return readClock16(CMD_ADJUSTMENT);
}

void Clock::setClockAdjustment(uint16_t value)
{
    writeClock16(CMD_ADJUSTMENT, value);
}

uint16_t Clock::getClockPosition()
{
    return readClock16(CMD_POSITION);
}

void Clock::setClockPosition(uint16_t value)
{
    writeClock16(CMD_POSITION, value);
}

uint8_t Clock::getClockTPDuration()
{
    return readClock8(CMD_TP_DURATION);
}

void Clock::setClockTPDuration(uint8_t value)
{
    writeClock8(CMD_TP_DURATION, value);
}

uint8_t Clock::getClockAPDuration()
{
    return readClock8(CMD_AP_DURATION);
}

void Clock::setClockAPDuration(uint8_t value)
{
    writeClock8(CMD_AP_DURATION, value);
}

uint8_t Clock::getClockAPDelay()
{
    return readClock8(CMD_AP_DELAY);
}

void Clock::setClockAPDelay(uint8_t value)
{
    writeClock8(CMD_AP_DELAY, value);
}

boolean Clock::getClockEnable()
{
    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    uint8_t value = Wire.read();
    return ((value & BIT_ENABLE) == BIT_ENABLE);
}

void Clock::setClockEnable(boolean enable)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    uint8_t value = Wire.read();

    if (enable)
    {
        value |= BIT_ENABLE;
    }
    else
    {
        value &= ~BIT_ENABLE;
    }

    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    Wire.write(value);
    Wire.endTransmission();
}

// send a command, read a 16 bit value
uint16_t Clock::readClock16(uint8_t command)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.endTransmission();
    uint16_t value;
    Wire.requestFrom(I2C_ADDRESS, sizeof(value));
    Wire.readBytes((uint8_t*) &value, sizeof(value));
    return (value);
}

// send a command with a 16 bit value
void Clock::writeClock16(uint8_t command, uint16_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.write((uint8_t*) &value, sizeof(value));
    Wire.endTransmission();
}

// send a command, read a 8 bit value
uint8_t Clock::readClock8(uint8_t command)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.endTransmission();
    uint8_t value;
    Wire.requestFrom(I2C_ADDRESS, sizeof(value));
    Wire.readBytes((uint8_t*) &value, sizeof(value));
    return (value);
}

// send a command with a 16 bit value
void Clock::writeClock8(uint8_t command, uint8_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.write(value);
    Wire.endTransmission();
}

