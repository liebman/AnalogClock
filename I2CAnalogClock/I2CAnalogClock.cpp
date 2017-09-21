/*
 * I2CAnalogClock.h
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

#include "I2CAnalogClock.h"

volatile uint16_t position; // This is the position that we believe the clock is in.
volatile uint16_t adjustment;   // This is the adjustment to be made.
volatile uint8_t tp_duration;
volatile uint8_t tp_duty;
volatile uint8_t ap_duration;
volatile uint8_t ap_duty;
volatile uint8_t ap_delay;     // delay in ms between ticks during adjustment

volatile uint8_t control;       // This is our control "register".
volatile uint8_t status;        // status register (has tick bit)

volatile uint8_t command;       // This is which "register" to be read/written.

volatile unsigned int pwm_duration;
volatile bool         adjust_active;
volatile unsigned int ticks;
volatile unsigned int id_count;

#if defined(PWRFAIL_PIN)
volatile bool         power_failed;
volatile uint8_t      pwrfail_control; // saved control register during power fail
#endif

// i2c receive handler
void i2creceive(int size)
{
    command = Wire.read();
    --size;
    // check for a write command
    if (size > 0)
    {
        switch (command)
        {
        case CMD_POSITION:
            position = Wire.read() | Wire.read() << 8;
            break;
        case CMD_ADJUSTMENT:
            adjustment = Wire.read() | Wire.read() << 8;
            // adjustment will start on the next tick!
            break;
        case CMD_TP_DURATION:
            tp_duration = Wire.read();
            break;
        case CMD_TP_DUTY:
            tp_duty = Wire.read();
            break;
        case CMD_AP_DURATION:
            ap_duration = Wire.read();
            break;
        case CMD_AP_DUTY:
            ap_duty = Wire.read();
            break;
        case CMD_AP_DELAY:
            ap_delay = Wire.read();
            break;
        case CMD_CONTROL:
            control = Wire.read();
            break;
        }
        command = 0xff;
    }
}

// i2c request handler
void i2crequest()
{
    uint16_t value;
    switch (command)
    {
    case CMD_ID:
        Wire.write(ID_VALUE);
        ++id_count;
        break;
    case CMD_POSITION:
        value = position;
        Wire.write(value & 0xff);
        Wire.write(value >> 8);
        break;
    case CMD_ADJUSTMENT:
        value = adjustment;
        Wire.write(value & 0xff);
        Wire.write(value >> 8);
        break;
    case CMD_TP_DURATION:
        value = tp_duration;
        Wire.write(value);
        break;
    case CMD_TP_DUTY:
        value = tp_duty;
        Wire.write(value);
        break;
    case CMD_AP_DURATION:
        value = ap_duration;
        Wire.write(value);
        break;
    case CMD_AP_DUTY:
        value = ap_duty;
        Wire.write(value);
        break;
    case CMD_AP_DELAY:
        value = ap_delay;
        Wire.write(value);
        break;
    case CMD_CONTROL:
        Wire.write(control);
        break;
    case CMD_STATUS:
        Wire.write(status);
        break;
    }
    command = 0xff;
}

void (*timer_cb)();
volatile bool timer_running;
volatile unsigned int start_time;

void clearTimer()
{
#if defined(__AVR_ATtinyX5__)
    TCCR1 = 0;
    GTCCR = 0;
    TIMSK &= ~(_BV(TOIE1) | _BV(OCIE1A) | _BV(OCIE1A));
#else
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
#endif
    OCR1A  = 0;
    OCR1B  = 0;
}

ISR(TIMER1_OVF_vect)
{
    if (pwm_duration == 0)
    {
        return;
    }

    pwm_duration -= 1;
    if (pwm_duration == 0)
    {
        clearTimer();
        timer_running = false;
        if (timer_cb != NULL)
        {
            timer_cb();
        }
    }
}

ISR(TIMER1_COMPA_vect)
{
    unsigned int int_time = millis();

    // why to we get an immediate interrupt? (ignore it)
    if (int_time == start_time || int_time == start_time+1)
    {
        return;
    }

#if defined(__AVR_ATtinyX5__)
    TIMSK &= ~(1 << OCIE1A); // disable timer1 interrupts as we only want this one.
#else
    TIMSK1 &= ~(1 << OCIE1A); // disable timer1 interrupts as we only want this one.
#endif
    timer_running = false;
    if (timer_cb != NULL)
    {
        timer_cb();
    }
}

void startTimer(int ms, void (*func)())
{
    start_time = millis();
    uint16_t timer = ms2Timer(ms);
    // initialize timer1
    noInterrupts();
    // disable all interrupts
    timer_cb = func;
#if defined(__AVR_ATtinyX5__)
    TCCR1 = 0;
    TCNT1 = 0;

    OCR1A = timer;   // compare match register
    TCCR1 |= (1 << CTC1);// CTC mode
    TCCR1 |= PRESCALE_BITS;
    TIMSK |= (1 << OCIE1A);// enable timer compare interrupt
    // clear any already pending interrupt?  does not work :-(
    TIFR &= ~(1 << OCIE1A);
#else
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;

    OCR1A = timer;   // compare match register
    TCCR1B |= (1 << WGM12);   // CTC mode
    TCCR1B |= PRESCALE_BITS;
    TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
    // clear any already pending interrupt?  does not work :-(
    TIFR1 &= ~(1 << OCIE1A);
#endif
    timer_running = true;
    interrupts();
    // enable all interrupts
}

void startPWM(unsigned int duration, unsigned int duty, void (*func)())
{
    noInterrupts();
    timer_cb = func;

    clearTimer();

    TCNT1 = 200; // needed???

    if (isTick())
    {
#if defined(__AVR_ATtinyX5__)
        TCCR1 = _BV(COM1A1) | _BV(PWM1A) | PWM_PRESCALE_BITS;
#else
        TCCR1A =  _BV(COM1A1) | _BV(WGM10);
#endif
        OCR1A = duty2pwm(duty);
    }
    else
    {
#if defined(__AVR_ATtinyX5__)
        TCCR1 = PWM_PRESCALE_BITS;
        GTCCR = _BV(COM1B1) | _BV(PWM1B);
#else
        TCCR1A =  _BV(COM1B1) | _BV(WGM10);
#endif
        OCR1B = duty2pwm(duty);
    }

#if defined(__AVR_ATtinyX5__)
    TIMSK |= _BV(TOIE1);
#else
    TCCR1B = _BV(WGM12) | PWM_PRESCALE_BITS;
    TIMSK1 = _BV(TOIE1);
#endif

    pwm_duration = ms2PWMCount(duration);;
    timer_running = true;
    interrupts();
}

void endTick()
{
#ifdef TEST_MODE
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
#endif

    timer_cb = NULL;
    toggleTick();

    if (adjustment != 0)
    {
        adjustment--;
        if (adjustment != 0)
        {
            startTimer(ap_delay, &adjustClock);
        }
        else
        {
            //
            // we are done with adjustment, stop the timer
            // and schedule the sleep.
            //
            adjust_active = false;
        }
    }
    else
    {
        if (adjust_active)
        {
            adjust_active = false;
        }
    }
}

// advance the position
void advancePosition()
{
    position += 1;
    if (position >= MAX_SECONDS)
    {
        position = 0;
    }
}

void adjustClock()
{
    if (adjustment == 1)
    {
        // last adustment uses tick pulse settings
        advanceClock(tp_duration, tp_duty);
    }
    else
    {
        advanceClock(ap_duration, ap_duty);
    }
}

//
//  Advance the clock by one second.
//
void advanceClock(uint16_t duration, uint8_t duty)
{
    advancePosition();
    startPWM(duration, duty, &endTick);
}

void startAdjust()
{
    if (!adjust_active)
    {
        adjust_active = true;

        //
        // the first adjustment uses the tick pulse timing
        advanceClock(tp_duration, tp_duty);
    }
}

//
// ISR for 1hz interrupt
//
void tick()
{
    ++ticks;
#if defined(LED_PIN)
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
#endif
    if (isEnabled())
    {
        if (adjustment != 0)
        {
            ++adjustment;
            startAdjust();
        }
        else
        {
            advanceClock(tp_duration, tp_duty);
        }
    }
    else
    {
        if (adjustment != 0)
        {
            startAdjust();
        }
    }
}

#if defined(PWRFAIL_PIN)
void powerFail()
{
    power_failed = true;

    //
    // save the control register and clear the enable bit
    //
    pwrfail_control = control;
    control &= ~BIT_ENABLE;

    //
    // force any adjustment to finish up.
    //
    if (adjustment > 1)
    {
        adjustment = 1;
    }
}
#endif

void setup()
{
#if defined(USE_1MHZ)
    // Change to 1 MHz by changing clock prescaler to 2
    cli();
    CLKPR = (1<<CLKPCE); // Prescaler enable
    CLKPR = (1<<CLKPS0) | (1<<CLKPS1); // Clock division factor 8 (0011)
    sei();
#elif defined(USE_4MHZ)
    // Change to 4 MHz by changing clock prescaler to 2
    cli();
    CLKPR = (1<<CLKPCE); // Prescaler enable
    CLKPR = (1<<CLKPS0); // Clock division factor 2 (0001)
    sei();
#endif

#if defined(SERIAL_BAUD)
    Serial.begin(SERIAL_BAUD);
    Serial.println("");
    Serial.println("Startup!");
#endif

#if defined(LED_PIN)
    digitalWrite(LED_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
#endif

#ifndef TEST_MODE
    ADCSRA &= ~(1 << ADEN); // Disable ADC as we don't use it, saves ~230uA
    PRR |= (1 << PRADC);    // Turn off ADC clock
#endif

    tp_duration   = DEFAULT_TP_DURATION_MS;
    tp_duty       = DEFAULT_TP_DUTY;
    ap_duration   = DEFAULT_AP_DURATION_MS;
    ap_duty       = DEFAULT_AP_DUTY;
    ap_delay      = DEFAULT_AP_DELAY_MS;
    adjust_active = false;

#ifdef SKIP_INITIAL_ADJUST
    position      = 0;
    adjustment    = 0;
#else
    //
    // we need a single adjust at startup to insure that the clock motor
    // is synched as a tick/tock.  This first tick will "misfire" if the motor
    // is out of sync and after that will be in sync.
    position      = MAX_SECONDS - 1;
    adjustment    = 1;
#endif

#if defined(TEST_MODE) || defined(START_ENABLED)
    control       = BIT_ENABLE;
#else
    control       = 0;
#endif

#ifndef TEST_MODE
    Wire.begin(I2C_ADDRESS);
    Wire.onReceive(&i2creceive);
    Wire.onRequest(&i2crequest);
#endif

#if defined(PWRFAIL_PIN)
    //
    // restore clock state if there is power fail data
    //
    loadPowerFailData();

    //
    // setup the power fail interrupt
    //
    power_failed = false;
    pinMode(PWRFAIL_PIN, INPUT);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(PWRFAIL_PIN), &powerFail, FALLING);
#endif

    digitalWrite(A_PIN, TICK_OFF);
    digitalWrite(B_PIN, TICK_OFF);

    pinMode(A_PIN, OUTPUT);
    pinMode(B_PIN, OUTPUT);
#ifdef TEST_MODE
    digitalWrite(LED_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);
#else
    pinMode(INT_PIN, INPUT);
    attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(INT_PIN), &tick, FALLING);
#endif

}

#ifdef DEBUG_I2CAC
unsigned int last_print;
unsigned int sleep_count;
#endif

void loop()
{
#ifdef USE_SLEEP
#ifdef USE_POWER_DOWN_MODE
    //
    // conserve power if i2c is not active and
    // there is no timer/PWM running
    //
    if (!timer_running && !Wire.isActive()) {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    }
    else
    {
        set_sleep_mode(SLEEP_MODE_IDLE);
    }
#endif

#ifdef DEBUG_I2CAC
    sleep_count += 1;
#endif
    // sleep!
    sleep_enable();
    sleep_cpu();
    sleep_disable();
#endif

#if defined(PWRFAIL_PIN)
    //
    // power failed and any running timers have finished
    //
    if (power_failed && !timer_running)
    {
        //
        // write current clock state to EEPROM
        //
        savePowerFailData();

        //
        //  Now wait for power to return.  Its more likely that the
        // capacitor will run out first and the we will run from start up.
        //
        while (digitalRead(PWRFAIL_PIN) == 0)
        {
            sleep_enable();
            sleep_cpu();
            sleep_disable();
        }

        //
        // we can resume!!!
        //
        cli();
        power_failed = false;
        control      = pwrfail_control;
        sei();
    }
#endif

#ifdef DEBUG_I2CAC
    unsigned int now = ticks;
    char buffer[256];
    if (now != last_print && (now%10)==0)
    {
        last_print = now;
#ifdef DEBUG_I2C
        snprintf(buffer, 127, "receives:%d requests:%d errors: %d\n",
                receives, requests, errors);
        Serial.print(buffer);
#endif
    }
#endif

#ifndef USE_SLEEP
#ifdef TEST_MODE
    if (!timer_running)
    {
        tick();
        delay(1000-tp_duration);
    }
#else
    delay(100);
#endif
#endif
}

#if defined(PWRFAIL_PIN)
uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffff;
    while (length--)
    {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1)
        {
            bool bit = crc & 0x80000000;
            if (c & i)
            {
                bit = !bit;
            }
            crc <<= 1;
            if (bit)
            {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}


boolean loadPowerFailData()
{
    EEPowerFailData data;

    // Read struct from EEPROM
    unsigned int i;
    uint8_t* p = (uint8_t*) &data;
    for (i = 0; i < sizeof(data); ++i)
    {
        p[i] = EEPROM.read(i);
    }

    uint32_t crcOfData = calculateCRC32(((uint8_t*) &data.data), sizeof(data.data));
    if (crcOfData != data.crc)
    {
        return false;
    }

    position    = data.data.pfd_position;
    control     = data.data.pfd_control;
    status      = data.data.pfd_status;
    tp_duration = data.data.pfd_tp_duration;
    tp_duty     = data.data.pfd_tp_duty;
    ap_duration = data.data.pfd_ap_duration;
    ap_duty     = data.data.pfd_ap_duty;
    ap_delay    = data.data.pfd_ap_delay;

    return true;
}

void savePowerFailData()
{
    EEPowerFailData data;
    data.data.pfd_position    = position;
    data.data.pfd_control     = pwrfail_control;
    data.data.pfd_status      = status;
    data.data.pfd_tp_duration = tp_duration;
    data.data.pfd_tp_duty     = tp_duty;
    data.data.pfd_ap_duration = ap_duration;
    data.data.pfd_ap_duty     = ap_duty;
    data.data.pfd_ap_delay    = ap_delay;

    data.crc = calculateCRC32(((uint8_t*) &data.data), sizeof(data.data));

    unsigned int i;
    uint8_t* p = (uint8_t*) &data;
    for (i = 0; i < sizeof(data); ++i)
    {
        EEPROM.write(i, p[i]);
    }
}

#endif
