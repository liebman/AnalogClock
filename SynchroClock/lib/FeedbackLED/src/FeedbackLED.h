/*
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
*/
#ifndef FeedbackLED_h
#define FeedbackLED_h

#include <Ticker.h>

#define FEEDBACK_LED_FAST   0.2
#define FEEDBACK_LED_MEDIUM 0.4
#define FEEDBACK_LED_SLOW   0.6

class FeedbackLED {
  public:
    FeedbackLED(int pin);
    void on();
    void off();
    void toggle();
    
    void blink(float rate);
    
  private:
    int    _pin;
    Ticker ticker;
    // static callback for ticker
    static void tick(FeedbackLED *fb);
};

#endif

