/*
 * UDP.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#include "UDPWrapper.h"

#include "Logger.h"
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>

UDPWrapper::UDPWrapper()
{
    _sockfd       = -1;
    _local_port   = -1;
}

UDPWrapper::~UDPWrapper()
{
    close();
}

int UDPWrapper::begin(int local_port)
{
    _local_port = local_port;
    return 0;
}

int UDPWrapper::open(IPAddress address, uint16_t port)
{
    struct sockaddr_in serv_addr; // Server address data structure.

    if (_local_port == -1)
    {
       dbprintln("UDP::open: begin() not called!");
       return -1;
    }

    _sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Create a UDP socket.

    if ( _sockfd < 0 )
    {
        dbprintln("socket() failed!");
        return -1;
    }

    bzero( ( char* ) &serv_addr, sizeof( serv_addr ) );

    serv_addr.sin_family = AF_INET;

    // Copy the server's IP address to the server address structure.

    serv_addr.sin_addr.s_addr = address;

    // Convert the port number integer to network big-endian style and save it to the server address structure.

    serv_addr.sin_port = htons(port);

    // Call up the server using its IP address and port number.

    if ( connect( _sockfd, ( struct sockaddr * ) &serv_addr, sizeof( serv_addr) ) < 0 )
    {
        dbprintf("connect() failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int UDPWrapper::send(void* buffer, size_t size)
{
    int n = ::write( _sockfd, ( char* ) buffer, size );

    if ( n < 0 )
    {
        dbprintf("write failed!  expected %d got %d\n", size, n);
    }
    return n;
}

int UDPWrapper::recv(void* buffer, size_t size, unsigned int timeout_ms)
{
    struct pollfd fd;
    int n;

    fd.fd = _sockfd;
    fd.events = POLLIN;
    n = ::poll(&fd, 1, timeout_ms);

    if (n == 0)
    {
        dbprintln("UDP::recv timeout!");
        return n;
    }

    n = ::read(_sockfd, buffer, size);
    return n;
}

int UDPWrapper::close()
{
    if (_sockfd != -1)
    {
        ::close(_sockfd);
    }
    return 0;
}
