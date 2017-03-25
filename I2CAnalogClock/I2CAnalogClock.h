// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _I2CAnalogClock_H_
#define _I2CAnalogClock_H_
#include "Arduino.h"

#ifdef __AVR_ATtinyX5__
#include "TinyWireS.h"
#else
#include "Wire.h"
#endif

//#define DEBUG_I2CAC

#ifdef __AVR_ATtinyX5__
#define INT_PIN         1
#define A_PIN           3
#define B_PIN           4
#define LED_PIN         LED_BUILTIN
#else
#define INT_PIN         3
#define A_PIN           4
#define B_PIN           5
#define LED_PIN         LED_BUILTIN
#endif

#define DRV8838         1

#ifdef DRV8838
#define DRV_PHASE       A_PIN
#define DRV_ENABLE      B_PIN
#endif

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
#define BIT_TICK        0x01

#define isEnabled()     (control &  BIT_ENABLE)
#define isTick()        (control &  BIT_TICK)
#define toggleTick()    (control ^= BIT_TICK)

#ifdef __AVR_ATtinyX5__
#define WireBegin(x)     TinyWireS.begin(x)
#define WireOnReceive(x) TinyWireS.onReceive(x)
#define WireOnRequest(x) TinyWireS.onRequest(x)
#define WireRead()       TinyWireS.receive()
#define WireWrite(x)     TinyWireS.send(x)
#else
#define WireBegin(x)     Wire.begin(x)
#define WireOnReceive(x) Wire.onReceive(x)
#define WireOnRequest(x) Wire.onRequest(x)
#define WireRead()       Wire.read()
#define WireWrite(x)     Wire.write(x)
#endif

//
// Timing defaults
//
#define DEFAULT_TP_DURATION 16000 // pulse duration in us.
#define DEFAULT_TP_COUNT    2     // pulse duration multiplier
#define DEFAULT_AP_DURATION 16000 // pulse duration during adjust
#define DEFAULT_AP_COUNT    2     // pulse duration multiplier during adjust
#define DEFAULT_AP_DELAY    16    // delay between adjust pulses in ms.

void tickDelay(uint16_t duration, uint16_t count);
void advanceClock(uint16_t duration, uint16_t count);
void tick();



//Do not add code below this line
#endif /* _I2CAnalogClock_H_ */
