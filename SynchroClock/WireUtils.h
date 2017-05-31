/*
 * WireUtils.h
 *
 *  Created on: May 26, 2017
 *      Author: liebman
 */

#ifndef WIREUTILS_H_
#define WIREUTILS_H_
#include <Arduino.h>
#include "Logger.h"

#define DEBUG_WIRE_UTILS

class WireUtilsC {
public:
	WireUtilsC();
	int clearBus();
};

extern WireUtilsC WireUtils;

#endif /* WIREUTILS_H_ */
