/*
 * SNTP.h
 *
 *  Created on: Mar 15, 2017
 *      Author: liebman
 */

#ifndef SNTP_H_
#define SNTP_H_
#include <Arduino.h>
#include <WiFiUdp.h>
#include <lwip/def.h> // htonl() & ntohl()

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
#define ms2Fraction(x) (uint32_t)((uint64_t)x*(uint64_t)(4294967296L)/(uint64_t)(1000L))
#define fraction2Ms(x) (uint32_t)((uint64_t)x/((uint64_t)(4294967296L)/(uint64_t)(1000L)))

typedef struct ntp_time {
  uint32_t seconds;
  uint32_t fraction;
} NTPTime, EpochTime;

typedef struct offset_time {
    int32_t  seconds;
    uint32_t fraction;
} OffsetTime;

typedef struct ntp_packet
{
  uint8_t  flags;
  uint8_t  stratum;
  uint8_t  poll;
  uint8_t  precision;
  uint32_t delay;
  uint32_t dispersion;
  uint8_t  ref_id[4];
  NTPTime  ref_time;
  NTPTime  orig_time;
  NTPTime  recv_time;
  NTPTime  xmit_time;
} SNTPPacket;

class SNTP
{
public:
  SNTP (const char* server, uint16_t port);
  void      begin(uint16_t local_port);
  EpochTime getTime(EpochTime now, OffsetTime* offset);

private:
  const char *server;
  uint16_t   port;
  WiFiUDP    udp;
  SNTPPacket  ntp;
};

#endif /* SNTP_H_ */
