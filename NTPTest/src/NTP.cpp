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

//#define NTP_DEBUG_PACKET

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

NTP::NTP(NTPPersist *persist)
{
    _persist = persist;
    _port    = NTP_PORT;
    dbprintf("****** sizeof(NTPPersist): %d\n", sizeof(NTPPersist));
}

void NTP::begin(int port, double drift)
{
    _port = port;
    _udp.begin(port);
    _persist->drift = drift;
}

double NTP::getDrift()
{
    return _persist->drift;
}
IPAddress NTP::getAddress()
{
    return _persist->ip;
}

// return next poll delay or -1 on error.
int NTP::getOffset(const char* server, double *offset, int (*getTime)(uint32_t *result))
{
    _persist->reach <<= 1;

    IPAddress address = _persist->ip;

    //
    // if we don't have a persisted ip address or the server name does not match the persisted one or
    // its not reachable then persist new ones.
    //
    if (_persist->ip == 0 || strncmp(server, _persist->server, NTP_SERVER_LENGTH) != 0 || _persist->reach == 0)
    {
        //dbprintln("NTP::getOffsetAndDelay: updating server and address!");
        if (!WiFi.hostByName(server, address))
        {
            dbprintf("NTP::getOffsetAndDelay: DNS lookup on %s failed!\n", server);
            return -1;
        }

        // we forget the existing data when we change NTP servers
        _persist->nsamples = 0;

        memset((void*)_persist->server, 0, sizeof(_persist->server ));
        strncpy(_persist->server, server, NTP_SERVER_LENGTH-1);
        _persist->ip = address;

        dbprintf("NTP::getOffsetAndDelay NEW server: %s address: %s\n", server, address.toString().c_str());
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
        dbprintln("failed to getTime() failed!");
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

    dbprintf("used server: %s address: %s\n", _persist->server, address.toString().c_str());
    dbprintf("packet size: %d\n", size);
    dbprintf("duration %ums\n", duration);

    if (size != 48)
    {
        dbprintln("bad packet!");
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
        dbprintf("packet returns err: %d\n", err);
        return err;
    }

    *offset = _persist->samples[0].offset;
    return 0;
}

int NTP::packet(NTPPacket* ntp, NTPTime now)
{
    if (ntp->stratum == 0)
    {
        dbprintln("bad stratum!");
        return -1;
    }

    if (getLI(ntp->flags) == LI_NOSYNC)
    {
        dbprintln("leap indicator indicates NOSYNC!");
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

    _persist->reach |= 1;

    uint64_t T1 = toUINT64(ntp->orig_time);
    uint64_t T2 = toUINT64(ntp->recv_time);
    uint64_t T3 = toUINT64(ntp->xmit_time);
    uint64_t T4 = toUINT64(now);

    double offset     = LFP2D(((int64_t)(T2 - T1) + (int64_t)(T3 - T4)) / 2);
    double delay      = LFP2D( (int64_t)(T4 - T1) - (int64_t)(T3 - T2));

    dbprintf("offset: %0.6lf delay: %0.6lf\n", offset, delay);

    int i;
    for (i = _persist->nsamples - 1; i >= 0; --i)
    {
        if (i == NTP_SAMPLE_COUNT - 1)
        {
            continue;
        }
        _persist->samples[i + 1] = _persist->samples[i];
        dbprintf("NTP::getOffsetAndDelay: samples[%d]: %lf delay:%lf timestamp:%u\n",
                i + 1, _persist->samples[i+1].offset, _persist->samples[i+1].delay, _persist->samples[i+1].timestamp);
    }

    _persist->samples[0].timestamp  = now.seconds;
    _persist->samples[0].offset     = offset;
    _persist->samples[0].delay      = delay;
    dbprintf("NTP::getOffsetAndDelay: samples[%d]: %lf delay:%lf timestamp:%u\n",
            0, _persist->samples[0].offset, _persist->samples[0].delay, _persist->samples[0].timestamp);

    if (_persist->nsamples < NTP_SAMPLE_COUNT)
    {
        _persist->nsamples += 1;
    }

    // compute the delay mean and std deviation

    double mean = 0.0;
    for (i = 0; i < _persist->nsamples; ++i)
    {
        mean = mean + _persist->samples[i].delay;
    }
    mean = mean / _persist->nsamples;
    double delay_std = 0.0;
    for (i = 0; i < _persist->nsamples; ++i)
    {
        delay_std = delay_std + pow(_persist->samples[i].delay - mean, 2);
    }
    delay_std = SQRT(delay_std / _persist->nsamples);
    dbprintf("STD DEV: %lf, mean: %lf\n", delay_std, mean);

    //
    // don't use this offset if its off of the mean by moth than one std deviation
    if ((fabs(_persist->samples[0].delay) - mean) > delay_std)
    {
        dbprintln("sample delay too big!");
        return -1;
    }

    //
    // don't use this offset if it does not meet the threshold
    //
    if (fabs(offset) < NTP_OFFSET_THRESHOLD)
    {
        dbprintln("offset not big enough for adjust!");
        return -1;
    }

    //
    // compute adjustment to drift
    //
    double drift_adjustment = clock();
    _persist->drift += drift_adjustment;

    //
    // sample ok to use.
    //
    return 0;
}

//
// process local clock, return drift
//
double NTP::clock()
{
    double drift = 0.0;

    if (_persist->nsamples >= NTP_SAMPLE_COUNT )
    {
        for (int i = _persist->nadjustments - 1; i >= 0; --i)
        {
            if (i == NTP_ADJUSTMENT_COUNT - 1)
            {
                continue;
            }
            _persist->adjustments[i + 1] = _persist->adjustments[i];
            dbprintf("NTP::getOffsetAndDelay: adjustments[%d]: %lf timestamp:%u\n",
                    i + 1, _persist->adjustments[i+1].adjustment, _persist->adjustments[i+1].timestamp);
        }

        // use the newest sample.
        _persist->adjustments[0].timestamp  = _persist->samples[0].timestamp;
        _persist->adjustments[0].adjustment = _persist->samples[0].offset;
        dbprintf("NTP::getOffsetAndDelay: adjustments[%d]: %lf timestamp:%u\n",
                0, _persist->adjustments[0].adjustment, _persist->adjustments[0].timestamp);

        if (_persist->nadjustments < NTP_ADJUSTMENT_COUNT)
        {
            _persist->nadjustments += 1;
        }

        //
        // calculate drift if we have all NTP_ADJUSTMENT_COUNT adjustments
        //
        if (_persist->nadjustments >= NTP_ADJUSTMENT_COUNT)
        {
            computeDrift(&drift);
        }
    }
    return drift;
}

int NTP::computeDrift(double* drift_result)
{
    double a = 0;
    double b = 0;
    double seconds = _persist->adjustments[0].timestamp - _persist->adjustments[_persist->nadjustments-1].timestamp;
    dbprintf("seconds: %f\n", seconds);
    for (int i = 0; i < _persist->nadjustments-1; ++i)
    {
        a += _persist->adjustments[i].adjustment;
        dbprintf("%0.12f\n", _persist->adjustments[i].adjustment);
        b += _persist->adjustments[i].adjustment * _persist->adjustments[i].adjustment;
    }
    a = a / seconds;
    b = SQRT(b/seconds);
    dbprintf("a: %f b:%f\n", a, b);
    double drift = a * 1000000;

    dbprintf("computeDrift: drift: %f ppm\n", drift);
    if (drift_result != NULL)
    {
        *drift_result = drift;
    }

    return 0;
}
