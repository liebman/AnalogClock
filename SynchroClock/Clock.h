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
    uint16_t getClockAdjustment();
    void setClockAdjustment(uint16_t value);
    uint16_t getClockPosition();
    void setClockPosition(uint16_t value);
    uint8_t getClockTPDuration();
    void setClockTPDuration(uint8_t value);
    uint8_t getClockAPDuration();
    void setClockAPDuration(uint8_t value);
    uint8_t getClockAPDelay();
    void setClockAPDelay(uint8_t value);
    boolean getClockEnable();
    void setClockEnable(boolean enable);
private:
    uint16_t readClock16(uint8_t command);
    void writeClock16(uint8_t command, uint16_t value);
    uint8_t readClock8(uint8_t command);
    void writeClock8(uint8_t command, uint8_t value);

};

#endif /* CLOCK_H_ */
