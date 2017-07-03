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

#include <Arduino.h>
#include <ESP8266WiFi.h>

#define USE_TCP
#define DEBUG_LOGGER

#ifdef USE_TCP
#include <WiFiClient.h>
#else
#include <WiFiUDP.h>
#endif

#define LOGGER_DEFAULT_BAUD 115200L
#define LOGGER_BUFFER_SIZE  256


class Logger {
public:
	Logger();
	void begin();
	void begin(long int baud);
	void end();
	void setNetworkLogger(const char* host, uint16_t port);
	void println(const char*message);
	void printf(const char*message, ...);
	void flush();

private:
#ifdef USE_TCP
	WiFiClient _client;
#else
    WiFiUDP     _udp;
#endif
    const char* _host;
    uint16_t    _port;
    uint16_t    _failed;
    char        _buffer[LOGGER_BUFFER_SIZE];

    void send(const char* message);
};

extern Logger logger;

#endif /* LOGGER_H_ */
