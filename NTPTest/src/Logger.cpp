/*
 * Logger.cpp
 *
 *  Created on: Jul 6, 2017
 *      Author: chris.l
 */

#include "Logger.h"
#include <stdio.h>
#include<stdarg.h>

Logger::Logger()
{
}

void Logger::begin(long int baud)
{
}

void Logger::end()
{
}

void Logger::setNetworkLogger(const char* host, unsigned short port)
{
}

void Logger::println(const char* message)
{
    printf("%s\n", message);
}

void Logger::printf(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);

    vprintf(fmt, argp);
    va_end(argp);
}

void Logger::flush()
{
    fflush(stdout);
}


Logger logger;
