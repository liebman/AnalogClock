/*
 * Logger.h
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
    char        _buffer[LOGGER_BUFFER_SIZE];

    void send(const char* message);
};

extern Logger logger;

#endif /* LOGGER_H_ */
