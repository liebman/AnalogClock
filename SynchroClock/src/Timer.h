/*
 * Timer.h
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#ifndef TIMER_H_
#define TIMER_H_
#include "Arduino.h"

class Timer
{
public:
    Timer();

    static uint32_t getMillis();
    void            start();
    uint32_t        stop();

private:
    static uint32_t _epoch;
    uint32_t        _start;
};

#endif /* TIMER_H_ */
