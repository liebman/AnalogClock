/*
 * Logger.cpp
 *
 *  Created on: May 31, 2017
 *      Author: liebman
 */

#include "Logger.h"


#ifdef DEBUG_LOGGER
#define DBP_BUF_SIZE 256

unsigned int snprintf(char*, unsigned int, ...);
#define dbprintf(...)  {char dbp_buf[DBP_BUF_SIZE]; snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprintln(x)   Serial.println(x)
#define dbflush()      Serial.flush()
#else
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#define dbflush()
#endif

Logger::Logger()
{
	_host       = NULL;
	_port       = 0;
}

void Logger::begin()
{
    begin(LOGGER_DEFAULT_BAUD);
}

void Logger::begin(long int baud)
{
    Serial.begin(baud);
}

void Logger::end()
{
#ifdef USE_TCP
    _client.stop();
#endif
}

void Logger::setNetworkLogger(const char* host, uint16_t port)
{
    _host = host;
    _port = port;
}

void Logger::println(const char* message)
{
    snprintf(_buffer, LOGGER_BUFFER_SIZE-1, "%s\n", message);
    Serial.print(_buffer);
    send(_buffer);
}

extern int vsnprintf(char* buffer, size_t size, const char* fmt, va_list args);

void Logger::printf(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);

    vsnprintf(_buffer, LOGGER_BUFFER_SIZE-1, fmt, argp);
    va_end(argp);

    Serial.print(_buffer);
    send(_buffer);
}

void Logger::flush()
{
    Serial.flush();
}

void Logger::send(const char* message)
{

    // if we are not configured for TCP then just return
    if (_host == NULL)
    {
        return;
    }

    IPAddress ip;

    if (!WiFi.isConnected())
    {
        dbprintln("Logger::log: not connected!");
        return;
    }

    if (!WiFi.hostByName(_host, ip))
    {
        dbprintf("Logger::log failed to resolve address for:%s\n", _host);
        return;
    }

#ifdef USE_TCP
    if (!_client.connected())
    {
        if(!_client.connect(ip, _port))
        {
            dbprintf("Logger::send failed to connect!\n");
            return;
        }
    }

    if (_client.write(message) != strlen(message))
    {
        dbprintf("Logger::write failed to write message: %s\n", message);
        return;
    }
    _client.flush();
#else
    _udp.beginPacket(ip, _port);
    _udp.write(message);
    if (!_udp.endPacket())
    {
        dbprintln("Logger::log failed to send packet!");
    }
#endif
}

Logger logger;
