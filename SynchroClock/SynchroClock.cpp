// Do not remove the include below
#include "SynchroClock.h"

#ifdef DS3231
RtcDS3231<TwoWire> rtc(Wire);
#endif
#ifdef DS1307
RtcDS1307<TwoWire> rtc(Wire);
#endif

FeedbackLED        feedback(LED_PIN);
ESP8266WebServer   HTTP(80);
SNTP               ntp("time.apple.com", 123);
Clock              clock;

bool    syncing  = false;
uint8_t last_pin = 0;

uint16_t getValidPosition(String name)
{
    int result = 0;

    char value[10];
    strncpy(value, HTTP.arg(name).c_str(), 9);
    if (strchr(value, ':') != NULL)
    {
        char* s = strtok(value, ":");

        if (s != NULL)
        {
            int h = atoi(s);
            while (h > 11)
            {
                h -= 12;
            }

            result += h * 3600; // hours to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s) * 60; // minutes to seconds
            s = strtok(NULL, ":");
        }
        if (s != NULL)
        {
            result += atoi(s);
        }
    }
    else
    {
        result = atoi(value);
        if (result < 0 || result > 43199)
        {
            Serial.println(
                    "invalid value for " + name + ": " + HTTP.arg(name)
                            + " using 0 instead!");
            result = 0;
        }
    }
    return result;
}

uint8_t getValidDuration(String name)
{
    int i = HTTP.arg(name).toInt();
    if (i < 0 || i > 255)
    {
        Serial.println(
                "invalid value for " + name + ": " + HTTP.arg(name)
                        + " using 32 instead!");
        i = 32;
    }
    return (uint8_t) i;
}

boolean getValidBoolean(String name)
{
    String value = HTTP.arg(name);
    return value.equalsIgnoreCase("true");
}

void handleAdjustment()
{
    uint16_t adj;
    if (HTTP.hasArg("set"))
    {
        if (HTTP.arg("set").equalsIgnoreCase("auto"))
        {
            Serial.println("Auto Adjust!");
            syncClockToRTC();
        }
        else
        {
            adj = getValidPosition("set");
            Serial.print("setting adjustment:");
            Serial.println(adj);
            clock.setAdjustment(adj);
        }
    }

    adj = clock.getAdjustment();

    HTTP.send(200, "text/plain", String(adj));
}

void handlePosition()
{
    uint16_t pos;
    if (HTTP.hasArg("set"))
    {
        pos = getValidPosition("set");
        Serial.print("setting position:");
        Serial.println(pos);
        clock.setPosition(pos);
    }

    pos = clock.getPosition();

    int hours = pos / 3600;
    int minutes = (pos - (hours * 3600)) / 60;
    int seconds = pos - (hours * 3600) - (minutes * 60);
    char message[64];
    sprintf(message, "%d (%02d:%02d:%02d)\n", pos, hours, minutes, seconds);
    HTTP.send(200, "text/Plain", message);
}

void handleTPDuration()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        Serial.print("setting tp_duration:");
        Serial.println(value);
        clock.setTPDuration(value);
    }

    value = clock.getTPDuration();

    HTTP.send(200, "text/plain", String(value));
}

void handleAPDuration()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        Serial.print("setting ap_duration:");
        Serial.println(value);
        clock.setAPDuration(value);
    }

    value = clock.getAPDuration();

    HTTP.send(200, "text/plain", String(value));
}

void handleAPDelay()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        Serial.print("setting ap_delay:");
        Serial.println(value);
        clock.setAPDelay(value);
    }

    value = clock.getAPDelay();

    HTTP.send(200, "text/plain", String(value));
}

void handleEnable()
{
    boolean enable;
    if (HTTP.hasArg("set"))
    {
        enable = getValidBoolean("set");
        clock.setEnable(enable);
    }
    enable = clock.getEnable();
    HTTP.send(200, "text/Plain", String(enable));
}

void handleRTC()
{
    uint16_t value = getRTCTimeAsPosition();
    HTTP.send(200, "text/plain", String(value));
}

void handleNTP()
{
    Serial.println("disabling the clock!");
    clock.setEnable(false);
    syncing = true;
    Serial.println("syncing now true!");
    HTTP.send(200, "text/Plain", "OK\n");
}

void setup()
{
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Startup!");

    pinMode(SYNC_PIN, INPUT);

    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);
    WiFiManager wifi;
    wifi.setAPCallback([](WiFiManager *)
    {   feedback.blink(FEEDBACK_LED_FAST);});
    String ssid = "SynchroClock" + String(ESP.getChipId());
    wifi.autoConnect(ssid.c_str(), NULL);
    feedback.off();

    rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

    if (!rtc.IsDateTimeValid())
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        rtc.SetDateTime(compiled);
    }

    if (!rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        rtc.SetIsRunning(true);
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
#ifdef DS3231
    rtc.Enable32kHzPin(false);
#endif

    boolean enabled = clock.getEnable();
    Serial.println("clock enable is:" + String(enabled));

    if (!enabled)
    {
#ifdef DS3231
        Serial.println("set 1Hz");
        rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
#endif
        Serial.println("starting square wave");
#ifdef DS3231
        rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
#endif
#ifdef DS1307
        rtc.SetSquareWavePin(DS1307SquareWaveOut_1Hz);
#endif
        Serial.println("enabling clock");
        clock.setEnable(true);
    }
    else
    {
        Serial.println("clock is enabled, skipping init of RTC");
    }

    Serial.println("starting HTTP");
    HTTP.on("/adjust", HTTP_GET, handleAdjustment);
    HTTP.on("/position", HTTP_GET, handlePosition);
    HTTP.on("/tp_duration", HTTP_GET, handleTPDuration);
    HTTP.on("/ap_duration", HTTP_GET, handleAPDuration);
    HTTP.on("/ap_delay", HTTP_GET, handleAPDelay);
    HTTP.on("/enable", HTTP_GET, handleEnable);
    HTTP.on("/rtc", HTTP_GET, handleRTC);
    HTTP.on("/ntp", HTTP_GET, handleNTP);
    HTTP.begin();
    ntp.begin(1235);
    syncing = false;
    last_pin = 0;
}

void loop()
{
    HTTP.handleClient();
    uint8_t pin = digitalRead(SYNC_PIN);
    if (syncing && last_pin == 1 && pin == 0)
    {
        EpochTime start;
        start.seconds = rtc.GetDateTime().Epoch32Time();
        start.fraction = 0;
        EpochTime end = ntp.getTime(start);

        // compute the delay to the next second

        uint32_t msdelay = 1000 - (((uint64_t) end.fraction * 1000) >> 32);
        Serial.print("msdelay: ");
        Serial.println(msdelay);
        // wait for the next second
        if (msdelay > 0 && msdelay < 1000)
        {
            delay(msdelay);
        }
        RtcDateTime dt(end.seconds + 1); // +1 because we waited for the next second
        rtc.SetDateTime(dt);
        syncing = false;
        last_pin = 0;
        delay(500);
        Serial.println("rtc updated, starting clock!");
        clock.setEnable(true);
        Serial.println("syncing clock to RTC");
        syncClockToRTC();
    }
    else
    {
        last_pin = pin;
    }
}

void syncClockToRTC()
{
    uint16_t rtc_pos = getRTCTimeAsPosition();
    Serial.printf("RTC position:%d", rtc_pos);
    uint16_t clock_pos = clock.getPosition();
    Serial.printf("clock position:%d", clock_pos);
    if (clock_pos != rtc_pos)
    {
        int adj = rtc_pos - clock_pos;
        if (adj < 0)
        {
            adj += 43200;
        }
        Serial.printf("sending adjustment of %d\n", adj);
        clock.setAdjustment(adj);
    }
}

//
// RTC functions
//
uint16_t getRTCTimeAsPosition()
{
    RtcDateTime time = rtc.GetDateTime();
    uint16_t hour = time.Hour();
    uint16_t minute = time.Minute();
    uint16_t second = time.Second();
    if (hour > 11)
    {
        hour -= 12;
        Serial.printf("%02d:%02d:%02d (CORRECTED)\n", hour, minute, second);
    }
    else
    {
        Serial.printf("%02d:%02d:%02d\n", hour, minute, second);
    }
    uint16_t position = hour * 60 * 60 + minute * 60 + second;
    return position;
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"), dt.Month(), dt.Day(),
            dt.Year(), dt.Hour(), dt.Minute(), dt.Second());
    Serial.print(datestring);
}
