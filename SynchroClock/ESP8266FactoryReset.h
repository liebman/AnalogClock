#ifndef FactoryReset_h
#define FactoryReset_h

#include <ESP8266WiFi.h>

class ESP8266FactoryReset
{
  public:
    ESP8266FactoryReset(int pin);

    typedef void (*callback_t)(void);

    void    setArmedCB(callback_t cb)    {_armed_cb    = cb;};
    void    setDisarmedCB(callback_t cb) {_disarmed_cb = cb;};
    void    setReadyCB(callback_t cb)    {_ready_cb    = cb;};
    void    setResetCB(callback_t cb)    {_reset_cb    = cb;};
    void    setResetDuration(unsigned long duration) {_reset_duration = duration;};
    
    void    setup();
    boolean loop();
    void    reset();
    boolean isArmed() {return _armed;}
    
  private:
    int           _pin;
    callback_t    _reset_cb       = NULL;
    callback_t    _ready_cb       = NULL;
    callback_t    _armed_cb       = NULL;
    callback_t    _disarmed_cb    = NULL;
    unsigned long _reset_duration = 5000; // 5 second default
    unsigned long _start_time     = 0;
    boolean       _armed          = false;
    boolean       _ready          = false;

    void    maybeSetReady();
    void    maybeResetOrClear();
    void    setArmed();
    void    clearArmed();
};

#endif

