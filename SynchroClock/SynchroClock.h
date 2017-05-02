// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _SynchroClock_H_
#define _SynchroClock_H_

#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include "FeedbackLED.h"
#include <Wire.h>
#include <ESP8266WebServer.h>
#include "SNTP.h"
#include "Clock.h"

#define DS3231
#define DEBUG_SYNCHRO_CLOCK

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

// pin definitions
#define LED_PIN         BUILTIN_LED
#define SYNC_PIN        D5          // pin tied to 1hz square wave from RTC

#define PIN_EDGE_RISING  1
#define PIN_EDGE_FALLING 0

void waitForEdge(int pin, int edge);

uint16_t getValidPosition(String name);
uint8_t getValidDuration(String name);
boolean getValidBoolean(String name);
void handleAdjustment();
void handlePosition();
void handleTPDuration();
void handleTPCount();
void handleAPDuration();
void handleAPCount();
void handleAPDelay();
void handleEnable();
void handleRTC();
void handleNTP();
void syncClockToRTC();
uint16_t getRTCTimeAsPosition();

#endif /* _SynchroClock_H_ */
