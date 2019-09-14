/*
 * SynchroClockVersion.h
 *
 *  Created on: Mar 24, 2018
 *      Author: chris.l
 */

#ifndef SYNCHROCLOCKVERSION_H_
#define SYNCHROCLOCKVERSION_H_

#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#ifndef VERSION_INFO
#define VERSION_INFO UNKNOWN
#endif

const char* SYNCHRO_CLOCK_VERSION = STRINGIZE(VERSION_INFO);

#endif /* SYNCHROCLOCKVERSION_H_ */
