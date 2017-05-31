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
#include <WiFiUDP.h>

#define DEBUG_LOGGER

#define LOGGER_DEFAULT_BAUD 115200L
#define LOGGER_BUFFER_SIZE  256

class Logger {
public:
	Logger();
	void begin();
	void begin(long int baud);
	void setNetworkLogger(const char* host, uint16_t port);
	void setNetworkLogger(const char* host, uint16_t port, uint16_t local_port);
	void println(const char*message);
	void printf(const char*message, ...);
	void flush();

private:
    WiFiUDP     _udp;
    const char* _host;
    uint16_t    _port;
    uint16_t    _local_port;
    char        _buffer[LOGGER_BUFFER_SIZE];

    void send(const char* message);
};

extern Logger logger;

#endif /* LOGGER_H_ */
