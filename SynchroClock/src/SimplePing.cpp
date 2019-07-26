/*
 * SimplePing.cpp
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#include "SimplePing.h"
#include <ping.h>

SimplePing::SimplePing()
{
    // TODO Auto-generated constructor stub

}


struct ping_option _po;
void SimplePing::ping(IPAddress server)
{
    _po.ip = server;
    _po.count = 1;
    _po.coarse_time = 1;
    _po.sent_function = NULL;
    _po.recv_function = NULL;
    ping_start(&_po);
    delay(100);  // time for arp and xmit
}
