// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _I2CAnalogClock_H_
#define _I2CAnalogClock_H_
#include "Arduino.h"
#include "Wire.h"

//#define DEBUG_I2CAC

#define INT_PIN         3
#define A_PIN           4
#define B_PIN           5
#define LED_PIN         LED_BUILTIN
#define MAX_SECONDS     43200

#define TICK_ON         HIGH
#define TICK_OFF        LOW

#define I2C_ADDRESS     0x09
#define CMD_POSITION    0x01
#define CMD_ADJUSTMENT  0x02
#define CMD_CONTROL     0x03
#define CMD_STATUS      0x04
#define CMD_TP_DURATION 0x05
#define CMD_TP_COUNT    0x06
#define CMD_AP_DURATION 0x07
#define CMD_AP_COUNT    0x08
#define CMD_AP_DELAY    0x09

// control register bits
#define BIT_ENABLE      0x80

#define isEnabled()     (control & BIT_ENABLE)
#define isTick()        (status  & BIT_TICK)
#define toggleTick()    (status ^= BIT_TICK)

//
// Timing defaults
//
#define DEFAULT_TP_DURATION 32000 // pulse duration in us.
#define DEFAULT_TP_COUNT    3     // pulse duration multiplier
#define DEFAULT_AP_DURATION 16000 // pulse duration during adjust
#define DEFAULT_AP_COUNT    3     // pulse duration multiplier during adjust
#define DEFAULT_AP_DELAY    16    // delay between adjust pulses in ms.

void tickDelay(uint16_t duration, uint16_t count);
void advanceClock(uint16_t duration, uint16_t count);
void tick();



//Do not add code below this line
#endif /* _I2CAnalogClock_H_ */
