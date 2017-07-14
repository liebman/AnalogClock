/*
 * UnixWiFi.h
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#ifndef UNIXWIFI_H_
#define UNIXWIFI_H_

#include "Types.h"
#include "IPAddress.h"

class UnixWiFi
{
public:
    UnixWiFi();
    int hostByName(const char* aHostname, IPAddress& aResult);

};

extern UnixWiFi WiFi;
#endif /* UNIXWIFI_H_ */
