/*
 * SNTP.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: liebman
 */

#include "SNTP.h"

#define toUINT64(x) (((uint64_t)(x.seconds)<<32) + x.fraction)

#ifdef SNTP_DEBUG
void dumpNTPPacket(SNTPPacket* ntp)
{

    Serial.printf("size:       %u\n",    sizeof(*ntp));
    Serial.printf("firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    Serial.printf("li:         %u\n",     getLI(ntp->flags));
    Serial.printf("version:    %u\n",     getVERS(ntp->flags));
    Serial.printf("mode:       %u\n",     getMODE(ntp->flags));
    Serial.printf("stratum:    %u\n",     ntp->stratum);
    Serial.printf("poll:       %u\n",     ntp->poll);
    Serial.printf("precision:  %u\n",     ntp->precision);
    Serial.printf("delay:      %u\n",     ntp->delay);
    Serial.printf("dispersion: %u\n",     ntp->dispersion);
    Serial.printf("ref_id:     %02x:%02x:%02x:%02x\n", ntp->ref_id[0], ntp->ref_id[1], ntp->ref_id[2], ntp->ref_id[3]);
    Serial.printf("ref_time:   %u:%u\n",  ntp->ref_time.seconds, ntp->ref_time.fraction);
    Serial.printf("orig_time:  %u:%u\n",  ntp->orig_time.seconds, ntp->orig_time.fraction);
    Serial.printf("recv_time:  %u:%u\n",  ntp->recv_time.seconds, ntp->recv_time.fraction);
    Serial.printf("xmit_time:  %u:%u\n",  ntp->xmit_time.seconds, ntp->xmit_time.fraction);
}


void print64(const char *label, uint64_t value)
{
    Serial.print(label);
    Serial.print((uint32_t)(value>>32));
    Serial.print(":");
    Serial.print((uint32_t)(value & 0xffffffff));
    Serial.println();
}

void print64s(const char *label, int64_t value)
{
    Serial.print(label);
    Serial.print((int32_t)(value>>32));
    Serial.print(":");
    Serial.print((uint32_t)(value & 0xffffffff));
    Serial.println();
}
#endif

OffsetTime computeOffset(NTPTime start, unsigned int millis_delta, SNTPPacket * ntp)
{

    uint64_t T1 = toUINT64(start);
    uint64_t T2 = toUINT64(ntp->recv_time);
    uint64_t T3 = toUINT64(ntp->xmit_time);
    uint64_t T4 = T1 + ms2Fraction(millis_delta);
    int64_t  o  = ((int64_t)(T2 - T1) + (int64_t)(T3 - T4)) / 2;

    OffsetTime offset;
    offset.seconds  = o >> 32;
    offset.fraction = o & 0xffffffff;

#ifdef SNTP_DEBUG
    int64_t  d  = (T4 - T1) - (T3 - T2);
    print64("T1:     ", T1);
    print64("T2:     ", T2);
    print64("T3:     ", T3);
    print64("T4:     ", T4);
    print64s("d:      ", d);
    print64s("o:      ", o);
    Serial.println();
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

#ifdef SNTP_DEBUG
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

#ifdef SNTP_DEBUG
  Serial.print("packet size:");
  Serial.println(size);
  Serial.print("attempt count:");
  Serial.println(count);
#endif

  if (size != 48) {
#ifdef SNTP_DEBUG
      Serial.println("bad packet!");
#endif
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

#ifdef SNTP_DEBUG
  dumpNTPPacket(&ntp);
#endif

  if (offset != NULL) {
      *offset = computeOffset(now, end - start, &ntp);
  }

  etime.seconds  = toEPOCH(ntp.xmit_time.seconds);
  etime.fraction = ntp.xmit_time.fraction;

  return etime;
}

