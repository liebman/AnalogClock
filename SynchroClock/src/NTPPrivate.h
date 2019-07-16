/*
 * NTPPrivate.h
 *
 *  Created on: Jul 6, 2017
 *      Author: chris.l
 */

#ifndef NTPPRIVATE_H_
#define NTPPRIVATE_H_

#define LI_NONE         0
#define LI_SIXTY_ONE    1
#define LI_FIFTY_NINE   2
#define LI_NOSYNC       3

#define MODE_RESERVED   0
#define MODE_ACTIVE     1
#define MODE_PASSIVE    2
#define MODE_CLIENT     3
#define MODE_SERVER     4
#define MODE_BROADCAST  5
#define MODE_CONTROL    6
#define MODE_PRIVATE    7

#define NTP_VERSION     4

#define MINPOLL         6       // % minimum poll interval (64 s)
#define MAXPOLL         17      /* % maximum poll interval (36.4 h) */


#define setLI(value)    ((value&0x03)<<6)
#define setVERS(value)  ((value&0x07)<<3)
#define setMODE(value)  ((value&0x07))

#define getLI(value)    ((value>>6)&0x03)
#define getVERS(value)  ((value>>3)&0x07)
#define getMODE(value)  (value&0x07)

#define SEVENTY_YEARS   2208988800L
#define toEPOCH(t)      ((uint32_t)t-SEVENTY_YEARS)
#define toNTP(t)        ((uint32_t)t+SEVENTY_YEARS)
#define toUINT64(x)     (((uint64_t)(x.seconds)<<32) + x.fraction)

#define FP2D(x)         ((double)(x)/65536)
#define LFP2D(x)        (((double)(x))/4294967296L)
#define ms2fraction(x)  ((uint32_t)((double)(x) / 1000.0 * (double)4294967296L))
#define LOG2D(a)        ((a) < 0 ? 1. / (1L << -(a)) : 1L << (a))
#define SQUARE(x)       ((x) * (x))
#define SQRT(x)         (sqrt(x))

//  simple versions - we don't worry about side effects
#define max(a, b)   ((a) < (b) ? (b) : (a))
#define min(a, b)   ((a) < (b) ? (a) : (b))

#endif /* NTPPRIVATE_H_ */
