/*
 *
 *
 * Copyright 2017 Christopher B. Liebman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 */
#include "Arduino.h"
#include "FeedbackLED.h"

FeedbackLED::FeedbackLED(int pin) {
  pinMode(pin, OUTPUT);
  _pin = pin;
}

void FeedbackLED::on() {
  ticker.detach();
  digitalWrite(_pin, LOW);
}

void FeedbackLED::off() {
  ticker.detach();
  digitalWrite(_pin, HIGH);
}

void FeedbackLED::toggle() {
  ticker.detach();
  int state = digitalRead(_pin);  // get the current state of LED pin
  digitalWrite(_pin, !state);     // set pin to the opposite state
}

void FeedbackLED::blink(float rate) {
  digitalWrite(_pin, LOW);
  ticker.attach(rate, tick, this);
}

void FeedbackLED::tick(FeedbackLED* fb) {
  int state = digitalRead(fb->_pin);  // get the current state of LED pin
  digitalWrite(fb->_pin, !state);     // set pin to the opposite state
}

