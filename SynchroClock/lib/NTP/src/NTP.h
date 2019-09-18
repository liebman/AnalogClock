/*
 * NTPProto.h
 *
 *  Created on: Jul 6, 2017
 *      Author: chris.l
 */

#ifndef NTP_H_
#define NTP_H_

#include "Arduino.h"
#include "SimplePing.h"
#include "Timer.h"
#include "UDPWrapper.h"
#include "Logger.h"

#define NTP_PORT 123

#ifndef NTP_REQUEST_COUNT
#define NTP_REQUEST_COUNT 1
#endif

typedef struct ntp_sample
{
    uint32_t timestamp;
    double   offset;
    double   delay;
} NTPSample;

typedef struct ntp_adjustment
{
    uint32_t timestamp;
    double adjustment;
} NTPAdjustment;

#define NTP_SERVER_LENGTH         64      // max length+1 of ntp server name
#define NTP_SAMPLE_COUNT          10      // number of NTP samples to keep for std devation filtering
#define NTP_ADJUSTMENT_COUNT      8       // number of NTP adjustments to keep for least squares drift
#define NTP_OFFSET_THRESHOLD      0.02    // 20ms offset minimum for adjust!
#ifndef NTP_MAX_INTERVAL
#define NTP_MAX_INTERVAL          129600  // 36 hours
#endif
#ifndef NTP_MIN_INTERVAL
#define NTP_MIN_INTERVAL          3600    // minimum computed interval
#endif
#ifndef NTP_SAMPLE_INTERVAL
#define NTP_SAMPLE_INTERVAL       (5*3600/(NTP_SAMPLE_COUNT)) // default to get the first set of samples in 4 hours
#endif
#ifndef NTP_UNREACH_LAST_INTERVAL
#define NTP_UNREACH_LAST_INTERVAL 3600    // last NTP was unreachable
#endif
#ifndef NTP_UNREACH_INTERVAL
#define NTP_UNREACH_INTERVAL      900     // last few NTP unreachable
#endif

//
//  Long term persisted data includes drift an last adjustment information
// so that we don't have to wait for a long time after power loss for drift
// to be active once we have computed it the first time.
//
typedef struct ntp_persist
{
    NTPAdjustment   adjustments[NTP_ADJUSTMENT_COUNT];
    int             nadjustments;
    double          drift;                              // computed drift in parts per million
} NTPPersist;

//
// This is used to validate new NTP responses and compute the clock drift
//
typedef struct ntp_runtime
{
    NTPSample       samples[NTP_SAMPLE_COUNT];
    int             nsamples;
    uint32_t        drift_timestamp;           // last time drift was applied
    double          drifted;                   // how much drift we have applied since the last NTP poll.
    uint32_t        update_timestamp;          // last time an update was applied
    double          drift_estimate;            // used to compute the poll interval
    double          poll_interval;             // estimated time between adjustments based on estimated drift
    double          delay_mean;                // mean value of sample delay
    double          delay_stddev;              // standard deviation of sample delay
    // cache these to know when we need to lookup the host again and if its been unreachable.
    char            server[NTP_SERVER_LENGTH]; // cached server name
    uint32_t        ip;                        // cached server ip address (only works for tcp v4)
    uint8_t         reach;
} NTPRunTime;

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
    NTP(NTPRunTime *runtime, NTPPersist *persist, void (*savePersist)(), int factor=1);
    void begin(int port = NTP_PORT);

    uint32_t getPollInterval();
    int getOffsetUsingDrift(double *offset, int (*getTime)(uint32_t *result));
    // return next poll delay or -1 on error.
    int getOffset(const char* server, double* offset, int (*getTime)(uint32_t *result));
    int getLastOffset(double* offset);
    IPAddress getAddress();
protected:
    int  makeRequest(IPAddress address, double *offset, double *delay, uint32_t *timestamp, int (*getTime)(uint32_t *result));
    int  makeRequest(IPAddress address, double *offset, double *delay, uint32_t *timestamp, int (*getTime)(uint32_t *result), const unsigned int bestof);
    int  process(uint32_t timestamp, double offset, double delay);
    void clock();
    int  computeDrift(double* drift_result);
    void updateDriftEstimate();
private:
    NTPRunTime *_runtime;
    NTPPersist *_persist;
    void      (*_savePersist)();
    UDPWrapper _udp;
    int        _port;
    int        _factor; // only used when testing to reduce fixed poll interval values by factor
};


#endif /* NTP_H_ */
