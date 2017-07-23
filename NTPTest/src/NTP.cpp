/*
 * NTPProto.cpp
 *
 *  Created on: Jul 6, 2017
 *      Author: chris.l
 */

#include "NTP.h"

#include "NTPPrivate.h"
#include <math.h>
#include <stdlib.h>
#ifdef ESP8266
#include <lwip/def.h> // htonl() & ntohl()
#endif

#define DEBUG
//#define NTP_DEBUG_PACKET
#include "Logger.h"


#ifdef NTP_DEBUG_PACKET
void dumpNTPPacket(NTPPacket* ntp)
{
    dbprintf("size:       %u\n", sizeof(*ntp));
    dbprintf("firstbyte:  0x%02x\n", *(uint8_t*)ntp);
    dbprintf("li:         %u\n", getLI(ntp->flags));
    dbprintf("version:    %u\n", getVERS(ntp->flags));
    dbprintf("mode:       %u\n", getMODE(ntp->flags));
    dbprintf("stratum:    %u\n", ntp->stratum);
    dbprintf("poll:       %u\n", ntp->poll);
    dbprintf("precision:  %d\n", ntp->precision);
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

NTP::NTP(NTPRunTime *runtime, NTPPersist *persist, void (*savePersist)(), int factor)
{
    _runtime     = runtime;
    _persist     = persist;
    _savePersist = savePersist;
    _port        = NTP_PORT;
    _factor      = factor;
    dbprintf("****** sizeof(NTPRunTime): %d\n", sizeof(NTPRunTime));
}

void NTP::begin(int port)
{
    _port   = port;
    _udp.begin(port);
    dbprintf("NTP::begin: nsamples: %d nadjustments: %d, drift: %f\n", _runtime->nsamples, _persist->nadjustments, _persist->drift);
}

IPAddress NTP::getAddress()
{
    return _runtime->ip;
}

int NTP::getLastOffset(double *offset)
{
    if (_runtime->nsamples > 0)
    {
        *offset = _runtime->samples[0].offset;
        return 0;
    }
    return -1;
}

uint32_t NTP::getPollInterval()
{
    //
    // if we don;t have all the samples yet, use a very short interval
    //
    if (_runtime->nsamples < NTP_SAMPLE_COUNT)
    {
        dbprintln("NTP::getPollInterval: samples not full, 15 minutes!");
        return 900 / _factor; // 15 minutes
    }

    //
    // if the last three polls failed use a very short interval
    //
    if ((_runtime->reach & 0x07) == 0)
    {
        dbprintln("NTP::getPollInterval: last three polls failed, using 15 minutes!");
        return 900 / _factor;
    }

    //
    // if the last poll failed then use a shorter interval
    //
    if ((_runtime->reach & 0x01) == 0)
    {
        dbprintln("NTP::getPollInterval: last poll failed, using 1 hour!");
        return 3600 / _factor;
    }

    //
    // if we don't have drift yet then use a shorter interval
    //
    if (_persist->nadjustments < NTP_ADJUSTMENT_COUNT)
    {
        dbprintln("NTP::getPollInterval: drift not calculated, using 1 hour!");
        return 3600 / _factor; // 1 hour
    }

    uint32_t seconds = _persist->adjustments[0].timestamp - _persist->adjustments[_persist->nadjustments-1].timestamp;
    seconds = (seconds / _persist->nadjustments-1) / 2;
    dbprintf("NTP::getPollInterval: we have drift compensation using avg interval between adjustments/2 %u\n", seconds);
    return seconds;
}

int NTP::getOffsetUsingDrift(double *offset_result, int (*getTime)(uint32_t *result))
{
    if (_persist->drift == 0.0)
    {
        dbprintln("NTP::getOffsetUsingDrift: not enough data to compute/use drift!");
        return -1;
    }

    uint32_t now;
    if (getTime(&now))
    {
        dbprintln("NTP::getOffsetUsingDrift: failed to getTime() failed!");
        return -1;
    }

    if (_runtime->drift_timestamp == 0)
    {
        dbprintln("NTP::getOffsetUsingDrift: first time, setting initial timestamp!");
        _runtime->drift_timestamp = now;
        return -1;
    }

    uint32_t interval = now - _runtime->drift_timestamp;
    double   offset   = (double)interval * _persist->drift / 1000000.0;
    dbprintf("NTP::getOffsetUsingDrift: interval: %u drift: %f offset: %f\n", interval, _persist->drift, offset);

    //
    // don't use this offset if it does not meet the threshold
    //
    if (fabs(offset) < NTP_OFFSET_THRESHOLD)
    {
        dbprintln("NTP::getOffsetUsingDrift: offset not big enough for adjust!");
        return -1;
    }

    *offset_result = offset;
    _runtime->drift_timestamp = now;
    _runtime->drifted += offset;
    return 0;
}

// return next poll delay or -1 on error.
int NTP::getOffset(const char* server, double *offset, int (*getTime)(uint32_t *result))
{
    _runtime->reach <<= 1;

    IPAddress address = _runtime->ip;

    //
    // if we don't have an ip address or the server name does not match the the one we have one or
    // its not reachable then lookup a new one.
    //
    if (_runtime->ip == 0 || strncmp(server, _runtime->server, NTP_SERVER_LENGTH) != 0 || _runtime->reach == 0)
    {
        //dbprintln("NTP::getOffset: updating server and address!");
        if (!WiFi.hostByName(server, address))
        {
            dbprintf("NTP::getOffset: DNS lookup on %s failed!\n", server);
            return -1;
        }


        memset((void*)_runtime->server, 0, sizeof(_runtime->server ));
        strncpy(_runtime->server, server, NTP_SERVER_LENGTH-1);
        _runtime->ip = address;

        dbprintf("NTP::getOffset: NEW server: %s address: %s\n", server, address.toString().c_str());

        // we forget the existing data when we change NTP servers
        _runtime->nsamples = 0;
    }

    //
    // Ping the server first, we don't care about the result.  This updates any
    // ARP cache etc.  Without this we see a varying 20ms -> 80ms delay on the
    // NTP packet.
    //
    Ping ping;
    ping.ping(address);

    dbflush();

    Timer timer;
    NTPTime now;
    NTPPacket ntp;

    memset((void*) &ntp, 0, sizeof(ntp));
    ntp.flags = setLI(LI_NONE) | setVERS(NTP_VERSION) | setMODE(MODE_CLIENT);
    ntp.poll  = MINPOLL;

    uint32_t start;
    if (getTime(&start))
    {
        dbprintln("NTP::getOffset: failed to getTime() failed!");
        return -1;
    }

    timer.start();

    now.seconds = toNTP(start);
    now.fraction = 0;
    // put non-zero timestamps in network byte order
    ntp.orig_time.seconds = htonl(ntp.orig_time.seconds);
    ntp.orig_time.fraction = htonl(ntp.orig_time.fraction);

    ntp.orig_time = now;

    dumpNTPPacket(&ntp);


    _udp.open(address, _port);
    _udp.send(&ntp, sizeof(ntp));

    memset(&ntp, 0, sizeof(ntp));

    int size = _udp.recv(&ntp, sizeof(ntp), 1000);
    uint32_t duration = timer.stop();

    dbprintf("NTP::getOffset: used server: %s address: %s\n", _runtime->server, address.toString().c_str());
    dbprintf("NTP::getOffset: packet size: %d\n", size);
    dbprintf("NTP::getOffset: duration %ums\n", duration);

    if (size != 48)
    {
        dbprintln("NTP::getOffset: bad packet!");
        return -1;
    }

    ntp.delay = ntohl(ntp.delay);
    ntp.dispersion = ntohl(ntp.dispersion);
    ntp.ref_time.seconds = ntohl(ntp.ref_time.seconds);
    ntp.ref_time.fraction = ntohl(ntp.ref_time.fraction);
    ntp.recv_time.seconds = ntohl(ntp.recv_time.seconds);
    ntp.recv_time.fraction = ntohl(ntp.recv_time.fraction);
    ntp.xmit_time.seconds = ntohl(ntp.xmit_time.seconds);
    ntp.xmit_time.fraction = ntohl(ntp.xmit_time.fraction);
    ntp.orig_time = now;

    dumpNTPPacket(&ntp);

    //
    // assumes start at second start and less than 1 second duration
    //
    now.fraction = ms2fraction(duration);

    int err = packet(&ntp, now);
    if (err)
    {
        dbprintf("NTP::getOffset: packet returns: %d\n", err);
        return err;
    }

    *offset = _runtime->samples[0].offset;
    return 0;
}

int NTP::packet(NTPPacket* ntp, NTPTime now)
{
    dbprintf("NTP::packet: nsamples: %d nadjustments: %d\n", _runtime->nsamples, _persist->nadjustments);
    if (ntp->stratum == 0)
    {
        dbprintln("NTP::packet: bad stratum!");
        return -1;
    }

    if (getLI(ntp->flags) == LI_NOSYNC)
    {
        dbprintln("NTP::packet: leap indicator indicates NOSYNC!");
        return -1; /* unsynchronized */
    }

    /*
     * Verify valid root distance.
     */
    //dbprintf("root distance: %lf reftime:%u xmit_time:%u\n", p->rootdelay / 2 + p->rootdisp, p->reftime_s, ntp->xmit_time.seconds);
    //if (p->rootdelay / 2 + p->rootdisp >= MAXDISP || p->reftime_s > ntp->xmit_time.seconds)
    //{
    //    dbprintln("invalid root distance or new time before prev time!");
    //    return -1;                 /* invalid header values */
    //}

    _runtime->reach |= 1;

    uint64_t T1 = toUINT64(ntp->orig_time);
    uint64_t T2 = toUINT64(ntp->recv_time);
    uint64_t T3 = toUINT64(ntp->xmit_time);
    uint64_t T4 = toUINT64(now);

    double offset     = LFP2D(((int64_t)(T2 - T1) + (int64_t)(T3 - T4)) / 2);
    double delay      = LFP2D( (int64_t)(T4 - T1) - (int64_t)(T3 - T2));

    dbprintf("NTP::packet: offset: %0.6lf delay: %0.6lf\n", offset, delay);

    int i;
    for (i = _runtime->nsamples - 1; i >= 0; --i)
    {
        if (i == NTP_SAMPLE_COUNT - 1)
        {
            continue;
        }
        _runtime->samples[i + 1] = _runtime->samples[i];
        dbprintf("NTP::packet: samples[%d]: %lf delay:%lf timestamp:%u\n",
                i + 1, _runtime->samples[i+1].offset, _runtime->samples[i+1].delay, _runtime->samples[i+1].timestamp);
    }

    _runtime->samples[0].timestamp  = now.seconds;
    _runtime->samples[0].offset     = offset;
    _runtime->samples[0].delay      = delay;
    dbprintf("NTP::packet: samples[%d]: %lf delay:%lf timestamp:%u\n",
            0, _runtime->samples[0].offset, _runtime->samples[0].delay, _runtime->samples[0].timestamp);

    if (_runtime->nsamples < NTP_SAMPLE_COUNT)
    {
        _runtime->nsamples += 1;
    }

    // compute the delay mean and std deviation

    double mean = 0.0;
    for (i = 0; i < _runtime->nsamples; ++i)
    {
        mean = mean + _runtime->samples[i].delay;
    }
    mean = mean / _runtime->nsamples;
    double delay_std = 0.0;
    for (i = 0; i < _runtime->nsamples; ++i)
    {
        delay_std = delay_std + pow(_runtime->samples[i].delay - mean, 2);
    }
    delay_std = SQRT(delay_std / _runtime->nsamples);
    dbprintf("NTP::packet: delay STD DEV: %lf, mean: %lf\n", delay_std, mean);

    //
    // don't use this offset if its off of the mean by moth than one std deviation
    if ((fabs(_runtime->samples[0].delay) - mean) > delay_std)
    {
        dbprintln("NTP::packet: sample delay too big!");
        return -1;
    }

    //
    // don't use this offset if it does not meet the threshold
    //
    if (fabs(offset) < NTP_OFFSET_THRESHOLD)
    {
        dbprintln("NTP::packet: offset not big enough for adjust!");
        return -1;
    }

    //
    // save adjustment samples & compute drift
    //
    clock();

    //
    // sample ok to use.
    //
    return 0;
}

//
// process local clock, return drift
//
void NTP::clock()
{
    if (_runtime->nsamples >= NTP_SAMPLE_COUNT )
    {
        for (int i = _persist->nadjustments - 1; i >= 0; --i)
        {
            if (i == NTP_ADJUSTMENT_COUNT - 1)
            {
                continue;
            }
            _persist->adjustments[i + 1] = _persist->adjustments[i];
            dbprintf("NTP::clock: adjustments[%d]: %lf timestamp:%u\n",
                    i + 1, _persist->adjustments[i+1].adjustment, _persist->adjustments[i+1].timestamp);
        }

        // use the newest sample and include any drift we have applied.
        _persist->adjustments[0].timestamp  = _runtime->samples[0].timestamp;
        _persist->adjustments[0].adjustment = _runtime->samples[0].offset + _runtime->drifted;
        _runtime->drifted = 0.0;
        dbprintf("NTP::clock: adjustments[%d]: %lf timestamp:%u\n",
                0, _persist->adjustments[0].adjustment, _persist->adjustments[0].timestamp);

        if (_persist->nadjustments < NTP_ADJUSTMENT_COUNT)
        {
            _persist->nadjustments += 1;
        }

        //
        // calculate drift if we have all NTP_ADJUSTMENT_COUNT adjustments
        //
        if (_persist->nadjustments >= 2)
        {
            computeDrift(&_persist->drift);
            dbprintf("NTP::clock: drift: %f\n", _persist->drift);
        }
        dbprintln("NTP::clock: saving 'persist' data!");
        _savePersist();
    }
}

//
// drift is the adjustment "per second" converted to parts per million
//
int NTP::computeDrift(double* drift_result)
{
    double a = 0;
    double seconds = _persist->adjustments[0].timestamp - _persist->adjustments[_persist->nadjustments-1].timestamp;
    dbprintf("seconds: %f\n", seconds);
    for (int i = 0; i < _persist->nadjustments-2; ++i)
    {
        a += _persist->adjustments[i].adjustment;
    }
    a = a / seconds;
    dbprintf("a: %f\n", a);
    double drift = a * 1000000;

    dbprintf("computeDrift: drift: %f PPM\n", drift);
    if (drift_result != NULL)
    {
        *drift_result = drift;
    }

    return 0;
}
