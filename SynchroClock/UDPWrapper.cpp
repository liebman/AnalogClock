/*
 * UDP.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#include "UDPWrapper.h"

//#define DEBUG
#include "Logger.h"

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
    dbprintf("UDPWrapper::open address:%u.%u.%u.%u:%u (local port: %d)\n",
            address[0], address[1], address[2], address[3], port, _local_port);

    if (!_udp.beginPacket(address, port))
    {
        dbprintln("UDPWrapper::open: beginPacket failed!");
        return 1;
    }
    return 0;
}

int UDPWrapper::send(void* buffer, size_t size)
{
    dbprintf("UDPWrapper::send: size:%u\n", size);
    int n = _udp.write((const uint8_t *) buffer, size);

    if ( n < 0 )
    {
        dbprintf("UDPWrapper::send: write failed!  expected %d got %d\n", size, n);
    }
    _udp.flush();

    if (!_udp.endPacket())
    {
        dbprintln("UDPWrapper::send: endPacket failed!");
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
        dbprintf("UDPWrapper::recv: failed wanted:%d != size:%d\n", wanted, size);
        return size;
    }

    _udp.read((char *)buffer, size);

    return size;
}

int UDPWrapper::close()
{
    dbprintln("UDPWrapper::close called!");
    _udp.stop();
    return 0;
}
