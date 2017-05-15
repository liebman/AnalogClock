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
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "FeedbackLED.h"
#include "SNTP.h"
#include "Clock.h"

#define DS3231
//#define DEBUG_SYNCHRO_CLOCK
//#define DISABLE_DEEP_SLEEP

#ifdef DS3231
#include <RtcDS3231.h>
#define SquareWavePin_ModeNone DS3231SquareWavePin_ModeNone
#endif
#ifdef DS1307
// hack because min is not defined :-/
#define min(x,y) _min(x,y)
#include <RtcDS1307.h>
#define SquareWavePin_ModeNone DS1307SquareWaveOut_High
#endif

#define EPOCH_1970to2000 946684800

#define MAX_SECONDS     43200 // seconds in 12 hours

// pin definitions
#define LED_PIN           D7          // D7 (GPIO13) when using device!
//#define LED_PIN           BUILTIN_LED // BUILTIN_LED when using dev board!
#define SYNC_PIN          D5          // (GPIO14) pin tied to 1hz square wave from RTC
#define FACTORY_RESET_PIN D6          // (GPIO12)

#define DEFAULT_TZ_OFFSET      0              // default timzezone offset in seconds
#define DEFAULT_NTP_SERVER     "pool.ntp.org" // default NTP server
#define DEFAULT_SLEEP_DURATION 3600           // default is 1hr


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
void syncClockToRTC();
uint16_t getRTCTimeAsPosition();
void saveConfig();
boolean loadConfig();

#endif /* _SynchroClock_H_ */
