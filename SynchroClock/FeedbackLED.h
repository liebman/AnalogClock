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

