/*
 * Clock.h
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

#ifndef CLOCK_H_
#define CLOCK_H_

#include <Arduino.h>
#include <Wire.h>
#include "WireUtils.h"
#include "Logger.h"

#define I2C_ADDRESS     0x09

#define CMD_ID          0x00
#define CMD_POSITION    0x01
#define CMD_ADJUSTMENT  0x02
#define CMD_CONTROL     0x03
#define CMD_STATUS      0x04
#define CMD_TP_DURATION 0x05
#define CMD_SAVE_CONFIG 0x06
#define CMD_AP_DURATION 0x07
#define CMD_AP_START    0x08
#define CMD_AP_DELAY    0x09
#define CMD_PWMTOP      0x0a
#define CMD_TP_DUTY     0x0b
#define CMD_AP_DUTY     0x0c
#define CMD_RESET       0x0d // factory reset
#define CMD_RST_REASON  0x0e // last reset reason
#define CMD_VERSION     0x0f // Clock firmware version

// control register bits
#define BIT_ENABLE      0x80

// status register bits
#define STATUS_BIT_TICK    0x01
#define STATUS_BIT_PWFBAD  0x80


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
    int begin(unsigned int retries);
    bool isClockPresent();
    int readAdjustment(uint16_t *value);
    int writeAdjustment(uint16_t value);
    int readPosition(uint16_t* value);
    int readPosition(uint16_t* value, unsigned int retries);
    int writePosition(uint16_t value);
    int readTPDuration(uint8_t* value);
    int writeTPDuration(uint8_t value);
    int readTPDuty(uint8_t* value);
    int writeTPDuty(uint8_t value);
    int readAPDuration(uint8_t* value);
    int writeAPDuration(uint8_t value);
    int readAPDuty(uint8_t* value);
    int writeAPDuty(uint8_t value);
    int readAPDelay(uint8_t* value);
    int writeAPDelay(uint8_t value);
    int readAPStartDuration(uint8_t* value);
    int writeAPStartDuration(uint8_t value);
    int readPWMTop(uint8_t* value);
    int writePWMTop(uint8_t value);
    int readStatus(uint8_t* value);
    int readStatus(uint8_t* value, unsigned int retries);
    int factoryReset();
    int readResetReason(uint8_t* value);
    int readResetReason(uint8_t* value, unsigned int retries);
    int readVersion(uint8_t* value);
    int readVersion(uint8_t* value, unsigned int retries);

    bool getEnable();
    void setEnable(bool enable);
    int saveConfig();
    bool getCommandBit(uint8_t);
    int setCommandBit(bool value, uint8_t bit);
    void waitForEdge(int edge);
private:
    int pin;
    int read(uint8_t  command, uint16_t *value);
    int write(uint8_t command, uint16_t  value);
    int read(uint8_t  command, uint8_t *value);
    int write(uint8_t command, uint8_t  value);
};

#endif /* CLOCK_H_ */
