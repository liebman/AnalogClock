/*
 * Ping.h
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#ifndef _SIMPLE_PING_H_
#define _SIMPLE_PING_H_
#include "Arduino.h"
#include <ESP8266WiFi.h>

class SimplePing
{
public:
    SimplePing();
    void ping(IPAddress address);
};
#endif /* _SIMPLE_PING_H_ */
