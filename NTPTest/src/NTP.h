/*
 * NTPProto.h
 *
 *  Created on: Jul 6, 2017
 *      Author: chris.l
 */

#ifndef NTP_H_
#define NTP_H_

#include "Arduino.h"
#include "Ping.h"
#include "Timer.h"
#include "UDPWrapper.h"

#define DEBUG
#include "Logger.h"

#define NTP_PORT 123

typedef double ntp_offset_t;

typedef struct ntp_sample
{
    uint32_t     timestamp;
    ntp_offset_t offset;
    double       delay;
} NTPSample;

typedef struct ntp_adjustment
{
    uint32_t timestamp;
    double adjustment;
} NTPAdjustment;

#define NTP_SERVER_LENGTH       64      // max length+1 of ntp server name
#define NTP_SAMPLE_COUNT        8       // number of NTP samples to keep for std devation filtering
#define NTP_ADJUSTMENT_COUNT    8       // number of NTP adjustments to keep for least squares drift
#define NTP_OFFSET_THRESHOLD    0.04    // 40ms offset minimum for adjust!

//
// This is used to validate new NTP responses and compute the clock drift
//
typedef struct ntp_persist
{
    NTPSample       samples[NTP_SAMPLE_COUNT];
    NTPAdjustment   adjustments[NTP_ADJUSTMENT_COUNT];
    int             nsamples;
    int             nadjustments;
    double          drift;
    // cache these to know when we need to lookup the host again and if its been unreachable.
    char            server[NTP_SERVER_LENGTH]; // cached server name
    uint32_t        ip;                        // cached server ip address (only works for tcp v4)
    uint8_t         reach;
} NTPPersist;

typedef struct ntp_time
{
    uint32_t seconds;
    uint32_t fraction;
} NTPTime;

typedef struct ntp_packet
{
    uint8_t  flags;
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t delay;
    uint32_t dispersion;
    uint8_t  ref_id[4];
    NTPTime  ref_time;
    NTPTime  orig_time;
    NTPTime  recv_time;
    NTPTime  xmit_time;
} NTPPacket;

class NTP
{
public:
    NTP(NTPPersist *persist);
    void begin(int port = NTP_PORT, double drift = 0.0);

    // return next poll delay or -1 on error.
    int getOffset(const char* server, double* offset, int (*getTime)(uint32_t *result));
    double getDrift();
    IPAddress getAddress();

private:
    NTPPersist *_persist;
    UDPWrapper        _udp;
    int        _port;

    int packet(NTPPacket* packet, NTPTime now);
    double clock();
    int computeDrift(double* drift_result);
};


#endif /* NTP_H_ */
