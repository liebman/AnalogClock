/*
 * SNTP.cpp
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
 *  Created on: Mar 15, 2017
 *      Author: liebman
 */

#include "SNTP.h"

#define toUINT64(x) (((uint64_t)(x.seconds)<<32) + x.fraction)

#ifdef SNTP_DEBUG
#define dbprintf(...) logger.printf(__VA_ARGS__)
#define dbprintln(x)  logger.println(x)
#define dbflush()     logger.flush()
#else
#define dbprintf(...)
#define dbprintln(x)
#define dbflush()
#endif

#ifdef SNTP_DEBUG_PACKET

void dumpNTPPacket(SNTPPacket* ntp)
{

    dbprintf("size:       %u\n",    sizeof(*ntp));
    dbprintf("firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    dbprintf("li:         %u\n",     getLI(ntp->flags));
    dbprintf("version:    %u\n",     getVERS(ntp->flags));
    dbprintf("mode:       %u\n",     getMODE(ntp->flags));
    dbprintf("stratum:    %u\n",     ntp->stratum);
    dbprintf("poll:       %u\n",     ntp->poll);
    dbprintf("precision:  %u\n",     ntp->precision);
    dbprintf("delay:      %u\n",     ntp->delay);
    dbprintf("dispersion: %u\n",     ntp->dispersion);
    dbprintf("ref_id:     %02x:%02x:%02x:%02x\n", ntp->ref_id[0], ntp->ref_id[1], ntp->ref_id[2], ntp->ref_id[3]);
    dbprintf("ref_time:   %08x:%08x\n",  ntp->ref_time.seconds, ntp->ref_time.fraction);
    dbprintf("orig_time:  %08x:%08x\n",  ntp->orig_time.seconds, ntp->orig_time.fraction);
    dbprintf("recv_time:  %08x:%08x\n",  ntp->recv_time.seconds, ntp->recv_time.fraction);
    dbprintf("xmit_time:  %08x:%08x\n",  ntp->xmit_time.seconds, ntp->xmit_time.fraction);
}
#endif

#ifdef SNTP_DEBUG

void print64(const char *label, uint64_t value)
{
    dbprintf("%s %08x:%08x (%Lf)\n", label, (uint32_t)(value>>32), (uint32_t)(value & 0xffffffff), ((long double)value / 4294967296L));
}

void print64s(const char *label, int64_t value)
{
    dbprintf("%s %08x:%08x (%Lf)\n", label, (int32_t)(value>>32), (uint32_t)(value & 0xffffffff), ((long double)value / 4294967296L));
}
#endif

OffsetTime computeOffset(NTPTime start, unsigned int millis_delta, SNTPPacket * ntp)
{

    uint64_t T1 = toUINT64(start);
    uint64_t T2 = toUINT64(ntp->recv_time);
    uint64_t T3 = toUINT64(ntp->xmit_time);
    uint64_t T4 = T1 + ms2Fraction(millis_delta);
    int64_t  o  = ((int64_t)(T2 - T1) + (int64_t)(T3 - T4)) / 2;
    OffsetTime offset = o;
    int64_t  d  = (T4 - T1) - (T3 - T2);
#ifdef SNTP_DEBUG
    dbprintf("delta:  %d\n", millis_delta);
    print64("T1:     ", T1);
    print64("T2:     ", T2);
    print64("T3:     ", T3);
    print64("T4:     ", T4);
    print64s("d:      ", d);
    print64s("o:      ", o);
#endif

    return offset;
}


SNTP::SNTP (const char* server, uint16_t port)
{
  this->default_server = server;
  this->port           = port;
}

void SNTP::begin(uint16_t local_port)
{
  udp.begin(local_port);
}

EpochTime SNTP::getTime(EpochTime now, OffsetTime* offset)
{
    return getTime(default_server, now, offset);
}

EpochTime SNTP::getTime(const char* server, EpochTime now, OffsetTime* offset)
{
    IPAddress address;
    if (WiFi.hostByName(server, address))
    {
        return getTime(address, now, offset);
    }
    EpochTime etime; // we will return this
    etime.seconds  = 0;
    etime.fraction = 0;
    return etime;
}

EpochTime SNTP::getTime(IPAddress server, EpochTime now, OffsetTime* offset)
{
  EpochTime etime; // we will return this

  // adjust now for NTP time
  now.seconds = toNTP(now.seconds);
  memset((void*)&ntp, 0, sizeof(ntp));
  ntp.flags     = setLI(LI_NONE) | setVERS(SNTP_VERSION) | setMODE(MODE_CLIENT);
  ntp.orig_time = now;

#ifdef SNTP_DEBUG_PACKET
  dumpNTPPacket(&ntp);
#endif

  // put timestamps in network byte order
  ntp.delay              = htonl(ntp.delay);
  ntp.dispersion         = htonl(ntp.dispersion);
  ntp.ref_time.seconds   = htonl(ntp.ref_time.seconds);
  ntp.ref_time.fraction  = htonl(ntp.ref_time.fraction);
  ntp.orig_time.seconds  = htonl(ntp.orig_time.seconds);
  ntp.orig_time.fraction = htonl(ntp.orig_time.fraction);
  ntp.recv_time.seconds  = htonl(ntp.recv_time.seconds);
  ntp.recv_time.fraction = htonl(ntp.recv_time.fraction);
  ntp.xmit_time.seconds  = htonl(ntp.xmit_time.seconds);
  ntp.xmit_time.fraction = htonl(ntp.xmit_time.fraction);

  unsigned int start = millis();
  // send it!
  udp.beginPacket(server, port);
  udp.write((const uint8_t *)&ntp, sizeof(ntp));
  udp.endPacket();

  memset((void*)&ntp, 0, sizeof(ntp));

  // wait for a packet for at most 1 second
  int size  = 0;
  int count = 0;
  while ((size = udp.parsePacket()) == 0) {
      ++count;
      if (millis() - start > 1000) {
	  break;
      }
  }

  dbprintf("packet size: %d\n", size);
  dbprintf("attempt count: %d\n", count);

  if (size != 48) {
      dbprintln("bad packet!");
      etime.seconds = 0;
      etime.fraction = 0;
      return etime;
  }

  udp.read((char *)&ntp, sizeof(ntp));
  unsigned int end = millis();

  // put timestamps in host byte order
  ntp.delay              = ntohl(ntp.delay);
  ntp.dispersion         = ntohl(ntp.dispersion);
  ntp.ref_time.seconds   = ntohl(ntp.ref_time.seconds);
  ntp.ref_time.fraction  = ntohl(ntp.ref_time.fraction);
  ntp.orig_time.seconds  = ntohl(ntp.orig_time.seconds);
  ntp.orig_time.fraction = ntohl(ntp.orig_time.fraction);
  ntp.recv_time.seconds  = ntohl(ntp.recv_time.seconds);
  ntp.recv_time.fraction = ntohl(ntp.recv_time.fraction);
  ntp.xmit_time.seconds  = ntohl(ntp.xmit_time.seconds);
  ntp.xmit_time.fraction = ntohl(ntp.xmit_time.fraction);

#ifdef SNTP_DEBUG_PACKET
  dumpNTPPacket(&ntp);
#endif

  if (offset != NULL) {
      *offset = computeOffset(now, end - start, &ntp);
  }

  etime.seconds  = toEPOCH(ntp.xmit_time.seconds);
  etime.fraction = ntp.xmit_time.fraction;

  return etime;
}

