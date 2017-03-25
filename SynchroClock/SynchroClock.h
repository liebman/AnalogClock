// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _SynchroClock_H_
#define _SynchroClock_H_
#include "Arduino.h"

#include "Arduino.h"
#include <WiFiManager.h>
#include "FeedbackLED.h"
#include <Wire.h>
#include <RtcDS3231.h>
#include <ESP8266WebServer.h>
#include "SNTP.h"

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

// pin definitions
#define LED_PIN         BUILTIN_LED
#define SYNC_PIN        D5          // pin tied to 1hz square wave from RTC

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
void printDateTime(const RtcDateTime& dt);
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
uint16_t readClock16(uint8_t command);
void writeClock16(uint8_t command, uint16_t value);
uint8_t readClock8(uint8_t command);
void writeClock8(uint8_t command, uint8_t value);

#endif /* _SynchroClock_H_ */
