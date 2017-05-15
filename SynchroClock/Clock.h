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

#define CMD_ID          0x00
#define CMD_POSITION    0x01
#define CMD_ADJUSTMENT  0x02
#define CMD_CONTROL     0x03
#define CMD_STATUS      0x04
#define CMD_TP_DURATION 0x05
#define CMD_AP_DURATION 0x07
#define CMD_AP_DELAY    0x09
#define CMD_SLEEP_DELAY 0x0a

// control register bits
#define BIT_ENABLE      0x80
#define BIT_STAY_ACTIVE 0x40

#define CLOCK_ID_VALUE  0x42

#define CLOCK_EDGE_RISING  1
#define CLOCK_EDGE_FALLING 0

class Clock
{
public:
    Clock(int _pin);
    bool isClockPresent();
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
    uint8_t getSleepDelay();
    void setSleepDelay(uint8_t value);
    bool getEnable();
    void setEnable(bool enable);
    bool getStayActive();
    void setStayActive(bool enable);
    bool getCommandBit(uint8_t);
    void setCommandBit(bool value, uint8_t);
    void waitForActive();
    void waitForEdge(int edge);
private:
    int pin;
    uint16_t read16(uint8_t command);
    void write16(uint8_t command, uint16_t value);
    uint8_t read8(uint8_t command);
    void write8(uint8_t command, uint8_t value);

};

#endif /* CLOCK_H_ */
