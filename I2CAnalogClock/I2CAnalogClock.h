// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _I2CAnalogClock_H_
#define _I2CAnalogClock_H_
#include "Arduino.h"
#include "PinChangeInterrupt.h"

#ifdef __AVR_ATtinyX5__
#include "TinyWireS.h"
#else
//#include "Wire.h"
#endif

//#define DEBUG_I2CAC     1
//#define DEBUG_I2C       1
//#define DEBUG_TIMER     1
#define DRV8838         1
#define SERIAL_BAUD     38400

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
#define CMD_AP_DURATION 0x07
#define CMD_AP_DELAY    0x09

// control register bits
#define BIT_ENABLE      0x80
#define BIT_TICK        0x01

#define isEnabled()     (control &  BIT_ENABLE)
#define isTick()        (control &  BIT_TICK)
#define toggleTick()    (control ^= BIT_TICK)

#ifdef __AVR_ATtinyX5__
#define WireRecType      uint8_t
#define WireBegin(x)     TinyWireS.begin(x)
#define WireOnReceive(x) TinyWireS.onReceive(x)
#define WireOnRequest(x) TinyWireS.onRequest(x)
#define WireRead()       TinyWireS.receive()
#define WireWrite(x)     TinyWireS.send(x)
#else
#define WireRecType      int
#define WireBegin(x)     Wire.begin(x)
#define WireOnReceive(x) Wire.onReceive(x)
#define WireOnRequest(x) Wire.onRequest(x)
#define WireRead()       Wire.read()
#define WireWrite(x)     Wire.write(x)
#endif

//
// Timing defaults
//


#ifdef __AVR_ATtinyX5__
#define CLOCK_HZ        8000000
#define PRESCALE        4096
#define PRESCALE_BITS   ((1 << CS13) | (1 << CS12) | (1 <<CS10)) // 4096 prescaler
#define ms2Timer(x) ((uint8_t)(CLOCK_HZ /(PRESCALE * (1/((double)x/1000)))))
#else
#define CLOCK_HZ        16000000
#define PRESCALE        256
#define PRESCALE_BITS   (1 << CS12) // 256 prescaler
#define ms2Timer(x) ((uint16_t)(CLOCK_HZ /(PRESCALE * (1/((double)x/1000)))))
#endif

#define DEFAULT_TP_DURATION_MS 16 // pulse duration in ms.
#define DEFAULT_AP_DURATION_MS 16 // pulse duration during adjust
#define DEFAULT_AP_DELAY_MS    8 // delay between adjust pulses in ms.




void advanceClock(uint16_t duration);
void tick();



//Do not add code below this line
#endif /* _I2CAnalogClock_H_ */
