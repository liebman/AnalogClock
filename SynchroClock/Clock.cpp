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
uint16_t Clock::getAdjustment()
{
    return read16(CMD_ADJUSTMENT);
}

void Clock::setAdjustment(uint16_t value)
{
    write16(CMD_ADJUSTMENT, value);
}

uint16_t Clock::getPosition()
{
    return read16(CMD_POSITION);
}

void Clock::setPosition(uint16_t value)
{
    write16(CMD_POSITION, value);
}

uint8_t Clock::getTPDuration()
{
    return read8(CMD_TP_DURATION);
}

void Clock::setTPDuration(uint8_t value)
{
    write8(CMD_TP_DURATION, value);
}

uint8_t Clock::getAPDuration()
{
    return read8(CMD_AP_DURATION);
}

void Clock::setAPDuration(uint8_t value)
{
    write8(CMD_AP_DURATION, value);
}

uint8_t Clock::getAPDelay()
{
    return read8(CMD_AP_DELAY);
}

void Clock::setAPDelay(uint8_t value)
{
    write8(CMD_AP_DELAY, value);
}

boolean Clock::getEnable()
{
    Wire.beginTransmission((uint8_t) I2C_ADDRESS);
    Wire.write(CMD_CONTROL);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) I2C_ADDRESS, (uint8_t) 1);
    uint8_t value = Wire.read();
    return ((value & BIT_ENABLE) == BIT_ENABLE);
}

void Clock::setEnable(boolean enable)
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
uint16_t Clock::read16(uint8_t command)
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
void Clock::write16(uint8_t command, uint16_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.write((uint8_t*) &value, sizeof(value));
    Wire.endTransmission();
}

// send a command, read a 8 bit value
uint8_t Clock::read8(uint8_t command)
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
void Clock::write8(uint8_t command, uint8_t value)
{
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(command);
    Wire.write(value);
    Wire.endTransmission();
}

