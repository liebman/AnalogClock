/*
 * SynchroClock.h
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
 *  Created on: May 26, 2017
 *      Author: liebman
 */

#ifndef _SynchroClock_H_
#define _SynchroClock_H_
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FeedbackLED.h"
#include "NTP.h"
#include "Clock.h"
#include "DS3231.h"
#include "WireUtils.h"
#include "TimeUtils.h"
#include "ConfigParam.h"

#define NTP_SET_RTC_THRESHOLD  0.04     // if offset greater than this update the RTC

#define USE_STOP_THE_CLOCK            // if defined then stop the clock for small negative adjustments
#define STOP_THE_CLOCK_MAX     60     // maximum difference where we will use stop the clock
#define STOP_THE_CLOCK_EXTRA   2      // extra seconds to leave the clock stopped

// pin definitions
#define LED_PIN           D7          // (GPIO13) LED on pin, active low
#define SYNC_PIN          D5          // (GPIO14) pin tied to 1hz square wave from RTC
#define FACTORY_RESET_PIN D6          // (GPIO12) button tied to pin

#define DEFAULT_TZ_OFFSET      0              // default timzezone offset in seconds
#define DEFAULT_NTP_SERVER     "pool.ntp.org" // default NTP server
#define DEFAULT_SLEEP_DURATION 3600           // default is 1hr

#define DEFAULT_TP_DURATION    24  // pulse duration in ms.
#define DEFAULT_AP_DURATION    16  // pulse duration during adjust
#define DEFAULT_AP_DELAY       12  // delay between adjust pulses in ms.
#define DEFAULT_SLEEP_DELAY    50  // delay before sleeping the DEV8838

#define MAX_SLEEP_DURATION     3600 // we do multiple sleep of this to handle bigger sleeps
#define CONNECTION_TIMEOUT     300  // wifi portal timeout - we will deep sleep and try again later

#define offset2longDouble(x)   ((long double)x / 4294967296L)

// error codes for setRTCfromNTP()
#define ERROR_DNS -1
#define ERROR_RTC -2
#define ERROR_NTP -3

#define TIME_CHANGE_COUNT  2

typedef struct
{
    uint32_t   sleep_duration;           // deep sleep duration in seconds
    int        tz_offset;                // time offset in seconds from UTC
    uint8_t    tp_duration;              // tick pulse duration in ms
    uint8_t    ap_duration;              // adjust pulse duration in ms
    uint8_t    ap_delay;                 // delay in ms between ticks during adjust
    uint8_t    sleep_delay;              // delay in ms before sleeping DR8838
    uint16_t    network_logger_port;     // port for network logging
    TimeChange tc[TIME_CHANGE_COUNT];    // time change description
    char       ntp_server[64];           // host to use for ntp
    char       network_logger_host[64];  // host for network logging
} Config;

typedef struct
{
    uint32_t crc;
    uint8_t data[sizeof(Config)];
} EEConfig;

typedef struct
{
    uint32_t sleep_delay_left;
    NTPPersist ntp_persist;
} DeepSleepData;

typedef struct
{
    uint32_t crc;
    uint8_t data[sizeof(DeepSleepData)];
} RTCDeepSleepData;

int parseOffset(const char* offset_string);
uint16_t parsePosition(const char* position_string);
int getValidOffset(String name);
uint16_t getValidPosition(String name);
uint8_t getValidDuration(String name);
boolean getValidBoolean(String name);
void handleOffset();
void handleAdjustment();
void handlePosition();
void handleTPDuration();
void handleTPCount();
void handleAPDuration();
void handleAPCount();
void handleAPDelay();
void handleSleepDelay();
void handleEnable();
void handleRTC();
void handleNTP();
int setRTCfromOffset(ntp_offset_t offset_ms, bool sync);
int setRTCfromNTP(const char* server, bool sync, ntp_offset_t* result_offset, IPAddress* result_address);
int setCLKfromRTC();
void saveConfig();
boolean loadConfig();
boolean readDeepSleepData();
boolean writeDeepSleepData();

extern unsigned int snprintf(char*, unsigned int, ...); // because esp8266 does not declare it in a header.

#endif /* _SynchroClock_H_ */
