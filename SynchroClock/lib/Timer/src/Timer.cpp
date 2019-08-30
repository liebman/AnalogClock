/*
 * Timer.cpp
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#include "Timer.h"
#include <sys/time.h>
#include "Logger.h"

uint32_t Timer::_epoch = 0;

Timer::Timer()
{
    _start = 0;
}

uint32_t Timer::getMillis()
{
    uint32_t ms = millis();
    return ms;
}

void Timer::start() {
    _start = getMillis();
}

uint32_t Timer::stop() {
    uint32_t stop = getMillis();
    return (stop - _start);
}
