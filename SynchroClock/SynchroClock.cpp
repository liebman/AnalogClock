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
SNTP               ntp("pool.ntp.org", 123);
Clock              clock;

#ifdef DEBUG_SYNCHRO_CLOCK
unsigned int snprintf(char*,unsigned int, ...);
#define DBP_BUF_SIZE 256
char    dbp_buf[DBP_BUF_SIZE];
#define dbbegin(x)    Serial.begin(x);
#define dbprintf(...) {snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprint(x) Serial.print(x)
#define dbprintln(x) Serial.println(x)
#else
#define dbbegin(x)
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#endif

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
            dbprintln("invalid value for " + name + ": " + HTTP.arg(name)
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
        dbprintln("invalid value for " + name + ": " + HTTP.arg(name)
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
            dbprintln("Auto Adjust!");
            syncClockToRTC();
        }
        else
        {
            adj = getValidPosition("set");
            dbprint("setting adjustment:");
            dbprintln(adj);
            clock.setAdjustment(adj);
        }
    }

    adj = clock.getAdjustment();

    HTTP.send(200, "text/plain", String(adj)+"\n");
}

void handlePosition()
{
    uint16_t pos;
    if (HTTP.hasArg("set"))
    {
        pos = getValidPosition("set");
        dbprint("setting position:");
        dbprintln(pos);
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
        dbprint("setting tp_duration:");
        dbprintln(value);
        clock.setTPDuration(value);
    }

    value = clock.getTPDuration();

    HTTP.send(200, "text/plain", String(value)+"\n");
}

void handleAPDuration()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprint("setting ap_duration:");
        dbprintln(value);
        clock.setAPDuration(value);
    }

    value = clock.getAPDuration();

    HTTP.send(200, "text/plain", String(value)+"\n");
}

void handleAPDelay()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprint("setting ap_delay:");
        dbprintln(value);
        clock.setAPDelay(value);
    }

    value = clock.getAPDelay();

    HTTP.send(200, "text/plain", String(value)+"\n");
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
    HTTP.send(200, "text/Plain", String(enable)+"\n");
}

void handleRTC()
{
    if (HTTP.hasArg("sync"))
    {
        syncClockToRTC();
    }
    uint16_t value = getRTCTimeAsPosition();
    int hours = value / 3600;
    int minutes = (value - (hours * 3600)) / 60;
    int seconds = value - (hours * 3600) - (minutes * 60);
    char message[64];
    sprintf(message, "%d (%02d:%02d:%02d)\n", value, hours, minutes, seconds);
    HTTP.send(200, "text/plain", message);
}

void waitForFallingEdge(int pin) {
    while (digitalRead(pin) != 1) {
        delay(1);
    }
    while (digitalRead(pin) != 0) {
        delay(1);
    }
}

#define PIN_EDGE_RISING  1
#define PIN_EDGE_FALLING 0

void waitForEdge(int pin, int edge) {
    while (digitalRead(pin) == edge) {
        delay(1);
    }
    while (digitalRead(pin) != edge) {
        delay(1);
    }
}

int getNTPOffset(const char* server, OffsetTime* offset)
{
    // Look up the address before we start
    IPAddress address;
    if (!WiFi.hostByName(server, address))
    {
        dbprintf("DNS lookup on %s failed!\n", server);
        return 0;
    }

    dbprintf("address: %s\n", address.toString().c_str());

    // wait for the next falling edge of the 1hz square wave
    waitForEdge(SYNC_PIN, PIN_EDGE_FALLING);

    RtcDateTime dt = rtc.GetDateTime();
    EpochTime start_epoch;
    start_epoch.seconds = dt.Epoch32Time();
    start_epoch.fraction = 0;
    EpochTime end = ntp.getTime(address, start_epoch, offset);

    if (end.seconds == 0) {
        return 0;
    }

    return 1;
}

void handleNTP() {
    char server[64] = "pool.ntp.org";
    boolean dryrun = false;

    if (HTTP.hasArg("server")) {
        dbprintf("SERVER: %s\n", HTTP.arg("server").c_str());
        strncpy(server, HTTP.arg("server").c_str(), 63);
        server[63] = 0;
    }

    if (HTTP.hasArg("dryrun")) {
        dbprintln("NTP dryrun!");
        dryrun = true;
    }

    dbprintf("using server: %s\n", server);

    // Look up the address before we start
    IPAddress address;
    if (!WiFi.hostByName(server, address))
    {
        dbprintf("DNS lookup on %s failed!\n", server);
        HTTP.send(500, "text/Plain", "DNS lookup failed!\n");
        return;
    }

    dbprintf("address: %s\n", address.toString().c_str());

    // wait for the next falling edge of the 1hz square wave
    waitForEdge(SYNC_PIN, PIN_EDGE_FALLING);

    RtcDateTime dt = rtc.GetDateTime();
    EpochTime start_epoch;
    start_epoch.seconds = dt.Epoch32Time();
    start_epoch.fraction = 0;
    dbprintf("start_epoch: %u.%u\n",  start_epoch.seconds, start_epoch.fraction);
    OffsetTime offset;
    EpochTime end = ntp.getTime(address, start_epoch, &offset);
    if (end.seconds == 0) {
        dbprintf("NTP Failed!\n");
        HTTP.send(500, "text/Plain", "NTP Failed!\n");
        return;
    }

    uint32_t offset_ms = fraction2Ms(offset.fraction);
    if (abs(offset.seconds) > 0 || offset_ms > 100)
    {
        dbprintf("offset > 100ms, updating RTC!\n");
        uint32_t msdelay = 1000 - offset_ms;
        dbprintf("msdelay: %u\n", msdelay);

        waitForEdge(SYNC_PIN, PIN_EDGE_FALLING);
        dt = rtc.GetDateTime();

        // wait for where the next second should start
        if (msdelay > 0 && msdelay < 1000)
        {
            delay(msdelay);
        }

        if (!dryrun)
        {
            dt += offset.seconds + 1; // +1 because we waited for the next second
            rtc.SetDateTime(dt);
            delay(100); // delay in case there was just a tick
            syncClockToRTC();
        }

    }
    char message[64];
    snprintf(message, 64, "OFFSET: %d.%03d (%s)\n", offset.seconds, offset_ms, address.toString().c_str());
    dbprintf(message);
    HTTP.send(200, "text/Plain", message);
}

void setup()
{
    dbbegin(115200);
    dbprintln("");
    dbprintln("Startup!");

    pinMode(SYNC_PIN, INPUT);

    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);
    WiFiManager wifi;
    wifi.setAPCallback([](WiFiManager *){feedback.blink(FEEDBACK_LED_FAST);});
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

        dbprintln("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        rtc.SetDateTime(compiled);
    }

    if (!rtc.GetIsRunning())
    {
        dbprintln("RTC was not actively running, starting now");
        rtc.SetIsRunning(true);
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
#ifdef DS3231
    rtc.Enable32kHzPin(false);
#endif

    boolean enabled = clock.getEnable();
    dbprintln("clock enable is:" + String(enabled));

    if (!enabled)
    {
        dbprintln("starting 1hz square wave");
#ifdef DS3231
        rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
        rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
#endif
#ifdef DS1307
        rtc.SetSquareWavePin(DS1307SquareWaveOut_1Hz);
#endif
        dbprintln("enabling clock");
        clock.setEnable(true);
    }
    else
    {
        dbprintln("clock is enabled, skipping init of RTC");
    }

    dbprintln("starting HTTP");
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
}

void loop()
{
    HTTP.handleClient();
    delay(100);
}

void syncClockToRTC()
{
    // TODO: should wait for falling 1hz edge!
    uint16_t rtc_pos = getRTCTimeAsPosition();
    dbprintf("RTC position:%d\n", rtc_pos);
    uint16_t clock_pos = clock.getPosition();
    dbprintf("clock position:%d\n", clock_pos);
    if (clock_pos != rtc_pos)
    {
        int adj = rtc_pos - clock_pos;
        if (adj < 0)
        {
            adj += 43200;
        }
        dbprintf("sending adjustment of %d\n", adj);
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
        dbprintf("%02d:%02d:%02d (CORRECTED)\n", hour, minute, second);
    }
    else
    {
        dbprintf("%02d:%02d:%02d\n", hour, minute, second);
    }
    uint16_t position = hour * 60 * 60 + minute * 60 + second;
    return position;
}
