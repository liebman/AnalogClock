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

