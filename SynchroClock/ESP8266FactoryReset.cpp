#include "ESP8266FactoryReset.h"

#include "Arduino.h"

extern "C" {
#include "user_interface.h" // this is for the RTC memory read/write functions
}

ESP8266FactoryReset::ESP8266FactoryReset(int pin)
{
  _pin = pin;
  pinMode(_pin, INPUT);
}

void ESP8266FactoryReset::setup() {
  
  while(digitalRead(_pin) == 0) {
    if (!_armed) {
      setArmed();
    } else {
      maybeSetReady();
    }
    yield();
  }
  
  maybeResetOrClear();
}

boolean ESP8266FactoryReset::loop() {
  
  if (digitalRead(_pin) == 0) {
    // button is pressed, arm if not already armed
    if(!_armed) {
      setArmed();
    } else {
      maybeSetReady();
    }
  } else {
    maybeResetOrClear();
  }
  return _armed;
}

void ESP8266FactoryReset::maybeSetReady() {
  if (!_ready && _armed && (millis() - _start_time) > _reset_duration) {
    Serial.println("marking ready!!!!");
    _ready = true;
    if (_ready_cb != NULL) {
      _ready_cb();
    }
  }
}

void ESP8266FactoryReset::maybeResetOrClear() {
  if (_ready) {
    // button is not pressed and we were armed and reset duration was met so reset!
    reset();
  } else if (_armed) {
    clearArmed();
  }
}


void ESP8266FactoryReset::setArmed() {
  _armed      = true;
  _start_time = millis();
  Serial.println("arming! start_time:"+String(_start_time));
  if (_armed_cb != NULL) {
    Serial.println("calling armed_cb");
    _armed_cb();
  }
}

void ESP8266FactoryReset::clearArmed() {
  Serial.println("clearing Armed!");
  _armed      = false;
  _start_time = 0;
  if (_disarmed_cb != NULL) {
    _disarmed_cb();
  }
}

void ESP8266FactoryReset::reset() {
  Serial.println("initiating factory reset!");
  if (_reset_cb != NULL) {
    Serial.println("calling reset_cb");
    _reset_cb();
  }
  WiFi.disconnect(true); // insure wifi disconnected.
  ESP.restart();
  delay(3000);
}
