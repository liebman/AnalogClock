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
//#define NTP_DEBUG_PACKET

typedef int64_t  ntp_offset_t;
typedef uint64_t ntp_delay_t;

#define offset2ms(x)           (ntp_offset_t)(x/((4294967296L)/(1000L)))
#define delay2ms(x)            (ntp_delay_t)(x/((4294967296L)/(1000L)))

typedef struct ntp_sample
{
    uint32_t     timestamp;
    ntp_offset_t offset;
    ntp_delay_t  delay;
} NTPSample;

#define NTP_SAMPLE_SIZE 8

//
// This is used to validate new NTP responses and compute the clock drift
//
typedef struct ntp_persist
{
    NTPSample    samples[NTP_SAMPLE_SIZE];
    unsigned int sample_count;
} NTPPersist;

class NTP
{
public:
  NTP(const char* server, uint16_t port, NTPPersist *persist);
  void       begin(uint16_t local_port);
  int        getOffsetAndDelay(IPAddress server, uint32_t now_seconds, ntp_offset_t* offset, ntp_delay_t *delay);  // returns -1 on error, 0 on success

private:
  int        ping(IPAddress server);
  const char *default_server;
  NTPPersist *persist;
  uint16_t   port;
  WiFiUDP    udp;
};

#endif /* SNTP_H_ */
