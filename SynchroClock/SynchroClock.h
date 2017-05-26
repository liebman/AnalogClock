// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _SynchroClock_H_
#define _SynchroClock_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include "twi.h"
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FeedbackLED.h"
#include "SNTP.h"
#include "Clock.h"
#include "DS3231.h"
#include "WireUtils.h"

//
// this enables Serial debugging output
//
#define DEBUG_SYNCHRO_CLOCK

//
// These are only used when debugging
//

//#define DISABLE_DEEP_SLEEP
//#define DISABLE_INITIAL_NTP
//#define DISABLE_INITIAL_SYNC

// pin definitions
#define LED_PIN           D7          // (GPIO13) LED on pin, active low
#define SYNC_PIN          D5          // (GPIO14) pin tied to 1hz square wave from RTC
#define FACTORY_RESET_PIN D6          // (GPIO12) button tied to pin

#define DEFAULT_TZ_OFFSET      0              // default timzezone offset in seconds
#define DEFAULT_NTP_SERVER     "pool.ntp.org" // default NTP server
#define DEFAULT_SLEEP_DURATION 3600           // default is 1hr

#define DEFAULT_TP_DURATION    12  // pulse duration in ms.
#define DEFAULT_AP_DURATION    16  // pulse duration during adjust
#define DEFAULT_AP_DELAY       12  // delay between adjust pulses in ms.
#define DEFAULT_SLEEP_DELAY    50  // delay before sleeping the DEV8838

// error codes for setRTCfromNTP()
#define ERROR_DNS -1
#define ERROR_RTC -2
#define ERROR_NTP -3

typedef struct
{
    int sleep_duration; // deep sleep duration in seconds
    int tz_offset;      // time offset in seconds from UTC
    uint8_t tp_duration;    // tick pulse duration in ms
    uint8_t ap_duration;    // adjust pulse duration in ms
    uint8_t ap_delay;       // delay in ms between ticks during adjust
    uint8_t sleep_delay;    // delay in ms before sleeping DR8838
    char ntp_server[64];
} Config;

typedef struct
{
    uint32_t crc;
    uint8_t data[sizeof(Config)];
} EEConfig;

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
int setRTCfromNTP(const char* server, bool sync, OffsetTime* result_offset, IPAddress* result_address);
void setCLKfromRTC();
void saveConfig();
boolean loadConfig();

#endif /* _SynchroClock_H_ */
