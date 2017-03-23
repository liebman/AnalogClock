/*
 * SNTP.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: liebman
 */

#include "SNTP.h"

void dumpNTPPacket(SNTPPacket* ntp)
{

    printf("size:       %u\n",    sizeof(*ntp));
    printf("firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    printf("li:         %u\n",     getLI(ntp->flags));
    printf("version:    %u\n",     getVERS(ntp->flags));
    printf("mode:       %u\n",     getMODE(ntp->flags));
    printf("stratum:    %u\n",     ntp->stratum);
    printf("poll:       %u\n",     ntp->poll);
    printf("precision:  %u\n",     ntp->precision);
    printf("delay:      %u\n",     ntp->delay);
    printf("dispersion: %u\n",     ntp->dispersion);
    printf("ref_id:     %02x:%02x:%02x:%02x\n", ntp->ref_id[0], ntp->ref_id[1], ntp->ref_id[2], ntp->ref_id[3]);
    printf("ref_time:   %u.%u\n",  ntp->ref_time.seconds, ntp->ref_time.fraction);
    printf("orig_time:  %u.%u\n",  ntp->orig_time.seconds, ntp->orig_time.fraction);
    printf("recv_time:  %u.%u\n",  ntp->recv_time.seconds, ntp->recv_time.fraction);
    printf("xmit_time:  %u.%u\n",  ntp->xmit_time.seconds, ntp->xmit_time.fraction);
}


SNTP::SNTP (const char* server, uint16_t port)
{
  this->server = server;
  this->port   = port;
}

void SNTP::begin(uint16_t local_port)
{
  udp.begin(local_port);
}

EpochTime SNTP::getTime(EpochTime now)
{
  EpochTime etime; // we will return this

  // adjust now for NTP time
  now.seconds = toNTP(now.seconds);

  memset((void*)&ntp, 0, sizeof(ntp));
  ntp.flags     = setLI(LI_NONE) | setVERS(SNTP_VERSION) | setMODE(MODE_CLIENT);
  ntp.orig_time = now;

  dumpNTPPacket(&ntp);

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

  // send it!
  udp.beginPacket(server, port);
  udp.write((const char *)&ntp, sizeof(ntp));
  udp.endPacket();

  memset((void*)&ntp, 0, sizeof(ntp));

  // wait for a packet for at most 1 second
  uint32_t start = millis();
  int size  = 0;
  int count = 0;
  while ((size = udp.parsePacket()) == 0) {
      ++count;
      if (millis() - start > 1000) {
	  break;
      }
  }

  Serial.print("packet size:");
  Serial.println(size);
  Serial.print("attempt count:");
  Serial.println(count);

  if (size != 48) {
      Serial.println("bad packet!");
      etime.seconds = 0;
      etime.fraction = 0;
      return etime;
  }

  udp.read((char *)&ntp, sizeof(ntp));

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

  dumpNTPPacket(&ntp);

  etime.seconds  = toEPOCH(ntp.xmit_time.seconds);
  etime.fraction = ntp.xmit_time.fraction;

  return etime;
}

