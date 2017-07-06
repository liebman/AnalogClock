/*
 * NTP.h
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

#ifndef NTP_H_
#define NTP_H_
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <lwip/def.h> // htonl() & ntohl()
#include "Logger.h"

#define NTP_DEBUG
#define NTP_DEBUG_PACKET

typedef double ntp_offset_t;
typedef double ntp_delay_t;
typedef double ntp_dispersion_t;

typedef struct ntp_sample
{
    uint32_t         timestamp;
    ntp_offset_t     offset;
    ntp_delay_t      delay;
    ntp_dispersion_t dispersion;
} NTPSample;

#define NTP_SERVER_LENGTH   64 // max length+1 of ntp server name
#define NTP_SAMPLE_COUNT    8  // number of NTP samples to keep

//
// This is used to validate new NTP responses and compute the clock drift
//
typedef struct ntp_persist
{
    NTPSample    samples[NTP_SAMPLE_COUNT];
    char         server[NTP_SERVER_LENGTH];
    double       drift;
    unsigned int sample_count;
    uint32_t     ip;
    uint8_t      reach;
} NTPPersist;

class NTP
{
public:
  NTP(uint16_t port, NTPPersist *persist);
  void       begin(uint16_t local_port);
  IPAddress  getServerAddress(); // get the last used server address
  int        getOffsetAndDelay(const char* server_name, uint32_t now_seconds, ntp_offset_t* offset, ntp_delay_t *delay);  // returns -1 on error, 0 on success

private:
  NTPPersist *persist;
  uint16_t   port;
  WiFiUDP    udp;

  int        ping(IPAddress server);
};

#endif /* SNTP_H_ */
