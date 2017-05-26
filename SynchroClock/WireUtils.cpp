/*
 * WireUtils.cpp
 *
 *  Created on: May 26, 2017
 *      Author: liebman
 */

#include "WireUtils.h"

#ifdef DEBUG_WIRE_UTILS
unsigned int snprintf(char*, unsigned int, ...);
#define DBP_BUF_SIZE 256
#define dbbegin(x)     Serial.begin(x);
#define dbprintf(...)  {char dbp_buf[DBP_BUF_SIZE]; snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprint(x)     Serial.print(x)
#define dbprintln(x)   Serial.println(x)
#define dbflush()      Serial.flush()
#else
#define dbbegin(x)
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#define dbflush()
#endif

WireUtilsC::WireUtilsC()
{
}

/**
 * This routine turns off the I2C bus and clears it
 * on return SCA and SCL pins are tri-state inputs.
 * You need to call Wire.begin() after this to re-enable I2C
 * This routine does NOT use the Wire library at all.
 *
 * returns 0 if bus cleared
 *         1 if SCL held low.
 *         2 if SDA held low by slave clock stretch for > 2sec
 *         3 if SDA held low after 20 clocks.
 */
int WireUtilsC::clearBus()
{
	dbprintln("WireUtils::ClearBus: attempting to clean i2c bus");
#if defined(TWCR) && defined(TWEN)
	TWCR &= ~(_BV(TWEN)); //Disable the Atmel 2-Wire interface so we can control the SDA and SCL pins directly
#endif
	pinMode(SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
	pinMode(SCL, INPUT_PULLUP);

	boolean SCL_LOW = (digitalRead(SCL) == LOW); // Check is SCL is Low.
	if (SCL_LOW)
	{ //If it is held low Arduno cannot become the I2C master.
		dbprintln("WireUtils::ClearBus: Failed! SCL held low!");
		return 1; //I2C bus error. Could not clear SCL clock line held low
	}

	boolean SDA_LOW = (digitalRead(SDA) == LOW);  // vi. Check SDA input.
	int clockCount = 20; // > 2x9 clock

	while (SDA_LOW && (clockCount > 0))
	{ //  vii. If SDA is Low,
		clockCount--;
		// Note: I2C bus is open collector so do NOT drive SCL or SDA high.
		pinMode(SCL, INPUT); // release SCL pullup so that when made output it will be LOW
		pinMode(SCL, OUTPUT); // then clock SCL Low
		delayMicroseconds(10); //  for >5uS
		pinMode(SCL, INPUT); // release SCL LOW
		pinMode(SCL, INPUT_PULLUP); // turn on pullup resistors again
		// do not force high as slave may be holding it low for clock stretching.
		delayMicroseconds(10); //  for >5uS
		// The >5uS is so that even the slowest I2C devices are handled.
		SCL_LOW = (digitalRead(SCL) == LOW); // Check if SCL is Low.
		int counter = 20;
		while (SCL_LOW && (counter > 0))
		{ //  loop waiting for SCL to become High only wait 2sec.
			counter--;
			delay(100);
			SCL_LOW = (digitalRead(SCL) == LOW);
		}
		if (SCL_LOW)
		{ // still low after 2 sec error
			dbprintln("WireUtils::ClearBus: Failed! SCL clock line held low by slave clock stretch for >2sec");
			return 2; // I2C bus error. Could not clear. SCL clock line held low by slave clock stretch for >2sec
		}
		SDA_LOW = (digitalRead(SDA) == LOW); //   and check SDA input again and loop
	}
	if (SDA_LOW)
	{ // still low
		dbprintln("WireUtils::ClearBus: Failed! SDA data line still held low");
		return 3; // I2C bus error. Could not clear. SDA data line held low
	}

	// else pull SDA line low for Start or Repeated Start
	pinMode(SDA, INPUT); // remove pullup.
	pinMode(SDA, OUTPUT); // and then make it LOW i.e. send an I2C Start or Repeated start control.
	// When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
	/// A Repeat Start is a Start occurring after a Start with no intervening Stop.
	delayMicroseconds(10); // wait >5uS
	pinMode(SDA, INPUT); // remove output low
	pinMode(SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
	delayMicroseconds(10); // x. wait >5uS
	pinMode(SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
	pinMode(SCL, INPUT);
	dbprintln("WireUtils::ClearBus: Success!");
	return 0; // all ok
}

// act like wire library (even though I don't like it)
WireUtilsC WireUtils = WireUtilsC();

