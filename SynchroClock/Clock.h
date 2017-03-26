/*
 * Clock.h
 *
 *  Created on: Mar 26, 2017
 *      Author: liebman
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <Arduino.h>
#include <Wire.h>

#define I2C_ADDRESS     0x09
#define CMD_POSITION    0x01
#define CMD_ADJUSTMENT  0x02
#define CMD_CONTROL     0x03
#define CMD_STATUS      0x04
#define CMD_TP_DURATION 0x05
#define CMD_AP_DURATION 0x07
#define CMD_AP_DELAY    0x09

// control register bits
#define BIT_ENABLE      0x80


class Clock
{
public:
    Clock();
    uint16_t getAdjustment();
    void setAdjustment(uint16_t value);
    uint16_t getPosition();
    void setPosition(uint16_t value);
    uint8_t getTPDuration();
    void setTPDuration(uint8_t value);
    uint8_t getAPDuration();
    void setAPDuration(uint8_t value);
    uint8_t getAPDelay();
    void setAPDelay(uint8_t value);
    boolean getEnable();
    void setEnable(boolean enable);
private:
    uint16_t read16(uint8_t command);
    void write16(uint8_t command, uint16_t value);
    uint8_t read8(uint8_t command);
    void write8(uint8_t command, uint8_t value);

};

#endif /* CLOCK_H_ */
