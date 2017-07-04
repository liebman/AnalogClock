/*
 * NTP.cpp
 *
 * Copyright 2017 Christopher B. Liebman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 *  Created on: July 2, 2017
 *      Author: liebman
 */

#define NTP_INTERNAL
#include "NTP.h"

#define LI_NONE        0
#define LI_SIXTY_ONE   1
#define LI_FIFTY_NINE  2
#define LI_ALARM       3

#define MODE_RESERVED  0
#define MODE_ACTIVE    1
#define MODE_PASSIVE   2
#define MODE_CLIENT    3
#define MODE_SERVER    4
#define MODE_BROADCAST 5
#define MODE_CONTROL   6
#define MODE_PRIVATE   7

#define SNTP_VERSION    4

#define setLI(value)      ((value&0x03)<<6)
#define setVERS(value)    ((value&0x07)<<3)
#define setMODE(value)    ((value&0x07))

#define getLI(value)      ((value>>6)&0x03)
#define getVERS(value)    ((value>>3)&0x07)
#define getMODE(value)    (value&0x07)

#define SEVENTY_YEARS     2208988800L
#define toEPOCH(t)        ((uint32_t)t-SEVENTY_YEARS)
#define toNTP(t)          ((uint32_t)t+SEVENTY_YEARS)
#define ms2Fraction(x)    (uint32_t)((uint64_t)x*(uint64_t)(4294967296L)/(uint64_t)(1000L))
#define fraction2Ms(x)    (uint32_t)((uint64_t)x/((uint64_t)(4294967296L)/(uint64_t)(1000L)))
#define toUINT64(x)       (((uint64_t)(x.seconds)<<32) + x.fraction)

typedef struct ntp_time
{
    uint32_t seconds;
    uint32_t fraction;
} NTPTime, EpochTime;

typedef struct ntp_packet
{
    uint8_t flags;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t delay;
    uint32_t dispersion;
    uint8_t ref_id[4];
    NTPTime ref_time;
    NTPTime orig_time;
    NTPTime recv_time;
    NTPTime xmit_time;
} NTPPacket;

#ifdef NTP_DEBUG
#define dbprintf(...)   logger.printf(__VA_ARGS__)
#define dbprint64(l,v)  logger.printf("%s %08x:%08x (%Lf)\n", l, (uint32_t)(v>>32), (uint32_t)(v & 0xffffffff), ((long double)v / 4294967296L))
#define dbprint64s(l,v) logger.printf("%s %08x:%08x (%Lf)\n", l,  (int32_t)(v>>32), (uint32_t)(v & 0xffffffff), ((long double)v / 4294967296L))
#define dbprintln(x)    logger.println(x)
#define dbflush()       logger.flush()
#else
#define dbprintf(...)
#define dbprint64(l,v)
#define dbprint64s(l,v)
#define dbprintln(x)
#define dbflush()

#endif

#ifdef NTP_DEBUG_PACKET
void dumpNTPPacket(SNTPPacket* ntp)
{

    dbprintf("size:       %u\n", sizeof(*ntp));
    dbprintf("firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    dbprintf("li:         %u\n", getLI(ntp->flags));
    dbprintf("version:    %u\n", getVERS(ntp->flags));
    dbprintf("mode:       %u\n", getMODE(ntp->flags));
    dbprintf("stratum:    %u\n", ntp->stratum);
    dbprintf("poll:       %u\n", ntp->poll);
    dbprintf("precision:  %u\n", ntp->precision);
    dbprintf("delay:      %u\n", ntp->delay);
    dbprintf("dispersion: %u\n", ntp->dispersion);
    dbprintf("ref_id:     %02x:%02x:%02x:%02x\n", ntp->ref_id[0], ntp->ref_id[1], ntp->ref_id[2], ntp->ref_id[3]);
    dbprintf("ref_time:   %08x:%08x\n", ntp->ref_time.seconds, ntp->ref_time.fraction);
    dbprintf("orig_time:  %08x:%08x\n", ntp->orig_time.seconds, ntp->orig_time.fraction);
    dbprintf("recv_time:  %08x:%08x\n", ntp->recv_time.seconds, ntp->recv_time.fraction);
    dbprintf("xmit_time:  %08x:%08x\n", ntp->xmit_time.seconds, ntp->xmit_time.fraction);
}
#else
#define dumpNTPPacket(x)
#endif

NTP::NTP(uint16_t port, NTPPersist *persist)
{
    memset((void*)persist->server, 0, sizeof(persist->server ));

    this->port    = port;
    this->persist = persist;
}

void NTP::begin(uint16_t local_port)
{
    udp.begin(local_port);
}


IPAddress NTP::getServerAddress()
{
    return persist->ip;
}

int NTP::getOffsetAndDelay(const char* server_name, uint32_t now_seconds, ntp_offset_t* offset, ntp_delay_t *delay)
{
    dbprintf("NTP::getOffsetAndDelay: previous reach: 0x%02x\n", persist->reach);
    persist->reach <<= 1;

    IPAddress address = persist->ip;
    //
    // if we don't have a persisted ip address or the server name does not match the persisted one or
    // its not reachable then persist new ones.
    //
    if (persist->ip == 0 || strncmp(server_name, persist->server, NTP_SERVER_LENGTH) != 0 || persist->reach == 0)
    {
        dbprintln("NTP::getOffsetAndDelay: updating server and address!");
        if (!WiFi.hostByName(server_name, address))
        {
            dbprintf("NTP::getOffsetAndDelay: DNS lookup on %s failed!\n", server_name);
            return -1;
        }

        memset((void*)persist->server, 0, sizeof(persist->server ));
        strncpy(persist->server, server_name, NTP_SERVER_LENGTH-1);
        persist->ip = address;

        dbprintf("NTP::getOffsetAndDelay NEW server: %s address: %s\n", server_name, address.toString().c_str());
    }

    NTPPacket ntp;
    NTPTime now;

    dbprintf("NTP::getOffsetAndDelay using server: %s address: %s\n", persist->server, address.toString().c_str());

    //
    // Ping the server first, we don't care about the result.  This updates any
    // ARP cache etc.  Without this we see a varying 20ms -> 80ms delay on the
    // NTP packet.
    //
    ping(address);

    // adjust now for NTP time
    now.seconds = toNTP(now_seconds);
    now.fraction = 0;

    memset((void*) &ntp, 0, sizeof(ntp));
    ntp.flags = setLI(LI_NONE) | setVERS(SNTP_VERSION) | setMODE(MODE_CLIENT);
    ntp.orig_time = now;

    dumpNTPPacket(&ntp);

    // put timestamps in network byte order
    ntp.delay = htonl(ntp.delay);
    ntp.dispersion = htonl(ntp.dispersion);
    ntp.ref_time.seconds = htonl(ntp.ref_time.seconds);
    ntp.ref_time.fraction = htonl(ntp.ref_time.fraction);
    ntp.orig_time.seconds = htonl(ntp.orig_time.seconds);
    ntp.orig_time.fraction = htonl(ntp.orig_time.fraction);
    ntp.recv_time.seconds = htonl(ntp.recv_time.seconds);
    ntp.recv_time.fraction = htonl(ntp.recv_time.fraction);
    ntp.xmit_time.seconds = htonl(ntp.xmit_time.seconds);
    ntp.xmit_time.fraction = htonl(ntp.xmit_time.fraction);

    unsigned int start = millis();
    // send it!
    udp.beginPacket(address, port);
    udp.write((const uint8_t *) &ntp, sizeof(ntp));
    udp.flush();
    udp.endPacket();
    unsigned int sent_by = millis();
    yield();
    unsigned int sent_ay = millis();

    memset((void*) &ntp, 0, sizeof(ntp));

    // wait for a packet for at most 1 second
    int size = 0;
    int count = 0;
    while ((size = udp.parsePacket()) == 0)
    {
        yield();
        ++count;
        if (millis() - start > 1000)
        {
            break;
        }
    }

    unsigned int end = millis();

    dbprintf("packet size: %d\n", size);
    dbprintf("attempt count: %d\n", count);

    if (size != 48)
    {
        dbprintln("bad packet!");
        return -1;
    }

    udp.read((char *) &ntp, sizeof(ntp));
    int duration = end - start;
    dbprintf("ntp request duration: %dms\n", duration);

    int duration_xmit = sent_by - start;
    int duration_yield = sent_ay - sent_by;
    dbprintf("xmit duration: %d yield: %d\n", duration_xmit, duration_yield);

    // put timestamps in host byte order
    ntp.delay = ntohl(ntp.delay);
    ntp.dispersion = ntohl(ntp.dispersion);
    ntp.ref_time.seconds = ntohl(ntp.ref_time.seconds);
    ntp.ref_time.fraction = ntohl(ntp.ref_time.fraction);
    ntp.orig_time.seconds = ntohl(ntp.orig_time.seconds);
    ntp.orig_time.fraction = ntohl(ntp.orig_time.fraction);
    ntp.recv_time.seconds = ntohl(ntp.recv_time.seconds);
    ntp.recv_time.fraction = ntohl(ntp.recv_time.fraction);
    ntp.xmit_time.seconds = ntohl(ntp.xmit_time.seconds);
    ntp.xmit_time.fraction = ntohl(ntp.xmit_time.fraction);

    dumpNTPPacket(&ntp);

    //
    // Compute offset and delay
    //

    uint64_t T1 = toUINT64(now);
    uint64_t T2 = toUINT64(ntp.recv_time);
    uint64_t T3 = toUINT64(ntp.xmit_time);
    uint64_t T4 = T1 + ms2Fraction(duration);
    int64_t o = ((int64_t) (T2 - T1) + (int64_t) (T3 - T4)) / 2;
    int64_t d = (T4 - T1) - (T3 - T2);
    dbprint64("T1:     ", T1);
    dbprint64("T2:     ", T2);
    dbprint64("T3:     ", T3);
    dbprint64("T4:     ", T4);
    dbprint64s("d:      ", d);
    dbprint64s("o:      ", o);

    //
    // update reach as we git a response.
    //
    persist->reach |= 1;
    dbprintf("NTP::getOffsetAndDelay: reach now: 0x%02x\n", persist->reach);

    int i;
    for (i = persist->sample_count - 1; i >= 0; --i)
    {
        if (i == NTP_SAMPLE_COUNT - 1)
        {
            continue;
        }
        persist->samples[i + 1] = persist->samples[i];
        dbprintf("NTP::getOffsetAndDelay: persist->samples[%d]: %.0lf (delay:%d)\n",
                i + 1, (double)(offset2ms(persist->samples[i+1].offset)),
                delay2ms(persist->samples[i+1].delay));
    }

    persist->samples[0].timestamp = now_seconds;
    persist->samples[0].offset = o;
    persist->samples[0].delay = d;

    dbprintf("getOffsetAndDelay: persist->samples[%d]: %.0lf (delay:%d)\n", 0,
            (double)(offset2ms(persist->samples[0].offset)),
            delay2ms(persist->samples[0].delay));

    if (persist->sample_count < NTP_SAMPLE_COUNT)
    {
        persist->sample_count += 1;
    }

    if (offset != NULL)
    {
        *offset = o;
    }
    if (delay != NULL)
    {
        *delay = d;
    }

    return 0;
}

extern "C" {
#include <user_interface.h>
#include <ping.h>
}

struct ping_option po;
int NTP::ping(IPAddress server)
{
    po.ip = server;
    po.count = 1;
    po.coarse_time = 1;
    po.sent_function = NULL;
    po.recv_function = NULL;
    ping_start(&po);
    delay(100);  // time for arp and xmit
    return 0;
}
