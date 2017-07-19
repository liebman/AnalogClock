# AnalogClock
Project to sync analog clocks to a few milliseconds using NTP for time synchronization.

Its a work in progress - more documentation coming soon-ish.

[I2CAnalogClock](I2CAnalogClock) contains the code for the ATTiny85 as an I2C based Analog Clock controller.  I used [Eclipse with an Arduino plugin](http://sloeber.io) for development.  If you want to use the Arduino IDE then create an empty file named I2CAnalogClock.ino.

[SynchroClock](SynchroClock) contains the code for the ESP8266 module.   I used [Eclipse with an Arduino plugin](http://sloeber.io) for development.  If you want to use the Arduino IDE then create an empty file named SynchroClock.ino.

[eagle](eagle) contains the [Eagle](https://www.autodesk.com/products/eagle/overview) design files.

## Arduino Board Requirements
* [I2CAnalogClock](I2CAnalogClock) uses ATTiny85 from [ATTinyCore](https://github.com/SpenceKonde/ATTinyCore) configured as internal 8mhz clock.
* [SynchroClock](SynchroClock) uses the 'NodeMCU 1.0 (ESP-12E Module)' from [ESP8266 core for Arduino](https://github.com/esp8266/Arduino) configured for 80mhz

## Arduino Library Requirements
* [WiFiManager](https://github.com/tzapu/WiFiManager) - by default WiFiManager only supports 10 extra fields on the configuration page. This project uses close to 30!  If my [pull request](https://github.com/tzapu/WiFiManager/pull/374) has not been merged then you will need to use my [fork](https://github.com/liebman/WiFiManager).

* [USIWire](https://github.com/puuu/USIWire) - This is the only i2c client implementation that worked properly with an ATTiny85 using its internal 8mhz clock.

