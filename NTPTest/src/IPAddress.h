/*
 * IPAddress.h
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#ifndef IPADDRESS_H_
#define IPADDRESS_H_
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include "Types.h"
#include "String.h"

class IPAddress
{
public:
    IPAddress();
    IPAddress(uint32_t address)
    {
        sin_addr.s_addr = address;
    }

    operator uint32_t() const {
        return sin_addr.s_addr;
    }

    IPAddress& operator=(const char *address) {
        memcpy(&sin_addr, address, sizeof(sin_addr));
        return *this;
    }

    IPAddress& operator=(uint32_t address) {
        sin_addr.s_addr = address;
        return *this;
    }

    String toString() const
    {
        char szRet[16];
        uint8_t *a =(uint8_t*)(&sin_addr.s_addr);
        sprintf(szRet,"%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
        return String(szRet);
    }
private:
    struct in_addr sin_addr; // Server address data structure.
};

#endif /* IPADDRESS_H_ */
