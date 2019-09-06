/*
 * UDP.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#include "UDPWrapper.h"

static PROGMEM const char TAG[] = "UDPWrapper";

UDPWrapper::UDPWrapper()
{
    _local_port   = -1;
}

UDPWrapper::~UDPWrapper()
{
    close();
}

int UDPWrapper::begin(int local_port)
{
    _local_port = local_port;
    return _udp.begin(local_port);
}

int UDPWrapper::open(IPAddress address, uint16_t port)
{
    dlog.debug(FPSTR(TAG), F("::open address:%u.%u.%u.%u:%u (local port: %d)"),
            address[0], address[1], address[2], address[3], port, _local_port);

    if (!_udp.beginPacket(address, port))
    {
        dlog.error(FPSTR(TAG), F("::open: beginPacket failed!"));
        return 1;
    }
    return 0;
}

int UDPWrapper::send(void* buffer, size_t size)
{
    dlog.debug(FPSTR(TAG), F("::send: size:%u"), size);
    size_t n = _udp.write((const uint8_t *) buffer, size);

    if ( n != size )
    {
        dlog.error(FPSTR(TAG), F("::send: write failed!  expected %d got %d"), size, n);
    }

    if (!_udp.endPacket())
    {
        dlog.error(FPSTR(TAG), F("::send: endPacket failed!"));
        return n;
    }
    return n;
}

int UDPWrapper::recv(void* buffer, size_t wanted, unsigned int timeout_ms)
{
    unsigned int start = millis();
    // wait for a packet for at most 1 second
    size_t size = 0;
    while ((size = _udp.parsePacket()) == 0)
    {
        yield();
        if (millis() - start > timeout_ms)
        {
            break;
        }
    }

    if (size != wanted)
    {
        dlog.error(FPSTR(TAG), F("::recv: failed wanted:%d != size:%d"), wanted, size);
        return size;
    }

    _udp.read((char *)buffer, size);

    return size;
}

int UDPWrapper::close()
{
    dlog.debug(FPSTR(TAG), F("::close called!"));
    _udp.stop();
    return 0;
}
