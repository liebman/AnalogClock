/*
 * UnixWiFi.cpp
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#include "UnixWiFi.h"
#include <netinet/in.h>
#include <netdb.h>

UnixWiFi::UnixWiFi()
{
    // TODO Auto-generated constructor stub

}

int UnixWiFi::hostByName(const char* aHostname, IPAddress& aResult)
{
    struct hostent *server;      // Server data structure.
    server = gethostbyname( aHostname ); // Convert URL to IP.
    if (server == NULL)
    {
        return 0;
    }
    aResult = server->h_addr_list[0];
    return 1;
}
UnixWiFi WiFi;
