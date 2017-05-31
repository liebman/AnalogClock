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
#include "WireUtils.h"
#include "Logger.h"

#define DEBUG_CLOCK
//#define DISABLE_ACTIVE
//#define DISABLE_EDGE


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

#define CLOCK_ERROR     0xffff
#define CLOCK_MAX       43200

class Clock
{
public:
    Clock(int _pin);
    int begin();
    bool isClockPresent();
    int readAdjustment(uint16_t *value);
    int writeAdjustment(uint16_t value);
    int readPosition(uint16_t* value);
    int writePosition(uint16_t value);
    int readTPDuration(uint8_t* value);
    int writeTPDuration(uint8_t value);
    int readAPDuration(uint8_t* value);
    int writeAPDuration(uint8_t value);
    int readAPDelay(uint8_t* value);
    int writeAPDelay(uint8_t value);
    int readSleepDelay(uint8_t* value);
    int writeSleepDelay(uint8_t value);
    void setSleepDelay(uint8_t value);
    bool getEnable();
    void setEnable(bool enable);
    bool getStayActive();
    void setStayActive(bool enable);
    bool getCommandBit(uint8_t);
    int setCommandBit(bool value, uint8_t bit);
    void waitForActive();
    void waitForEdge(int edge);
private:
    int pin;
    int read(uint8_t  command, uint16_t *value);
    int write(uint8_t command, uint16_t  value);
    int read(uint8_t  command, uint8_t *value);
    int write(uint8_t command, uint8_t  value);
};

#endif /* CLOCK_H_ */
