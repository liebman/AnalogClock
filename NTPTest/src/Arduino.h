/*
 * Types.h
 *
 *  Created on: Jul 7, 2017
 *      Author: chris.l
 */

#ifndef ARDUINO_H_
#define ARDUINO_H_

#include "Types.h"
#include "IPAddress.h"
#include "UnixWiFi.h"
#include "String.h"

#define yield()
#define millis() Timer::getMillis()
#endif /* ARDUINO_H_ */
