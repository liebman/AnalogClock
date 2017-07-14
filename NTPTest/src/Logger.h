/*
 * Logger.h
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
 *  Created on: May 31, 2017
 *      Author: liebman
 */

#ifndef LOGGER_H_
#define LOGGER_H_

class Logger {
public:
    Logger();
    void begin(long int baud = 115200L);
    void end();
    void setNetworkLogger(const char* host, unsigned short port);
    void println(const char*message);
    void printf(const char*message, ...);
    void flush();
};

extern Logger logger;

#define DEBUG
#ifdef DEBUG
#define dbprintf(...)   logger.printf(__VA_ARGS__)
#define dbprint64(l,v)  logger.printf("%s %08x:%08x (%Lf)\n", l, (uint32_t)(v>>32), (uint32_t)(v & 0xffffffff), ((long double)v / 4294967296.))
#define dbprint64s(l,v) logger.printf("%s %08x:%08x (%Lf)\n", l,  (int32_t)(v>>32), (uint32_t)(v & 0xffffffff), ((long double)v / 4294967296.))
#define dbprintln(x)    logger.println(x)
#define dbflush()       logger.flush()
#else
#define dbprintf(...)
#define dbprint64(l,v)
#define dbprint64s(l,v)
#define dbprintln(x)
#define dbflush()
#endif

#endif /* LOGGER_H_ */
