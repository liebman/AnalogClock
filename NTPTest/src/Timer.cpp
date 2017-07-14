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
    struct timeval tp;
    gettimeofday(&tp, NULL);
    if (_epoch == 0)
    {
        _epoch = tp.tv_sec;
    }
    //dbprintf("getMillis() %u:%u\n", tp.tv_sec, tp.tv_usec);

    uint32_t ms = (tp.tv_sec-_epoch) * 1000 + tp.tv_usec / 1000;
    return ms;
}

void Timer::start() {
    _start = getMillis();
}

uint32_t Timer::stop() {
    uint32_t stop = getMillis();
    return (stop - _start);
}
