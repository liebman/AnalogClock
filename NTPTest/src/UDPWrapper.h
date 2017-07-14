/*
 * UDP.h
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#ifndef UDPWRAPPER_H_
#define UDPWRAPPER_H_
#include "Arduino.h"

class UDPWrapper
{
public:
    UDPWrapper();
    virtual ~UDPWrapper();
    int begin(int local_port);
    int open(IPAddress address, uint16_t port);
    int send(void* buffer, size_t size);
    int recv(void* buffer, size_t size, unsigned int timeout_ms);
    int close();
private:
    int _local_port;
    int _sockfd;
};

#endif /* UDPWRAPPER_H_ */
