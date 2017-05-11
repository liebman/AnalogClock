// Do not remove the include below
#include "SynchroClock.h"

#ifdef DS3231
RtcDS3231<TwoWire> rtc(Wire);
#endif
#ifdef DS1307
RtcDS1307<TwoWire> rtc(Wire);
#endif

typedef struct {
    int      sleep_duration; // deep sleep duration in seconds
    int      tz_offset;      // time offset in seconds from UTC
    uint8_t  tp_duration;    // tick pulse duration in ms
    uint8_t  ap_duration;    // adjust pulse duration in ms
    uint8_t  ap_delay;       // delay in ms between ticks during adjust
    uint8_t  pad;
    char     ntp_server[64];
} Config;

typedef struct {
  uint32_t crc;
  uint8_t  data[sizeof(Config)];
} EEConfig;

Config       config;

FeedbackLED         feedback(LED_PIN);
ESP8266WebServer    HTTP(80);
SNTP                ntp("pool.ntp.org", 123);
Clock               clock;
WiFiManager wifi;
unsigned long       setup_done = 0;

boolean save_config  = false; // used by wifi manager when settings were updated.
boolean force_config = false; // reset handler sets this to force into config
boolean stay_awake   = false; // don't use deep sleep

#ifdef DEBUG_SYNCHRO_CLOCK
unsigned int snprintf(char*,unsigned int, ...);
#define DBP_BUF_SIZE 256
char    dbp_buf[DBP_BUF_SIZE];
#define dbbegin(x)    Serial.begin(x);
#define dbprintf(...) {snprintf(dbp_buf, DBP_BUF_SIZE-1, __VA_ARGS__); Serial.print(dbp_buf);}
#define dbprint(x)    Serial.print(x)
#define dbprintln(x)  Serial.println(x)
#define dbflush()     Serial.flush()
#else
#define dbbegin(x)
#define dbprintf(...)
#define dbprint(x)
#define dbprintln(x)
#define dbflush()
#endif


int parseOffset(const char* offset_string)
{
    int result = 0;
    char value[11];
    strncpy(value, offset_string, 10);
    if (strchr(value, ':') != NULL)
    {
        int sign = 1;
        char* s;

        if (value[0] == '-') {
            sign = -1;
            s = strtok(&(value[1]), ":");
        }
        else
        {
            s = strtok(value, ":");
        }
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
        // apply sign
        result *= sign;
    }
    else
    {
        result = atoi(value);
        if (result < -43199 || result > 43199)
        {
            result = 0;
        }
    }
    return result;
}

uint16_t parsePosition(const char* position_string)
{
    int result = 0;
    char value[1];
    strncpy(value, position_string, 9);
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
            result = 0;
        }
    }
    return result;
}

int getValidOffset(String name)
{
    int result = parseOffset(HTTP.arg(name).c_str());

    return result;
}

uint16_t getValidPosition(String name)
{
    int result = parsePosition(HTTP.arg(name).c_str());

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

void handleOffset()
{
    if (HTTP.hasArg("set"))
    {
        config.tz_offset = getValidOffset("set");
        dbprint("seconds offset:");
        dbprintln(config.tz_offset);
        saveConfig();
    }

    HTTP.send(200, "text/plain", String(config.tz_offset)+"\n");
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
            waitForEdge(SYNC_PIN, PIN_EDGE_RISING);
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

int setTimeFromNTP(const char* server, bool sync, OffsetTime* result_offset, IPAddress* result_address)
{
    dbprintf("using server: %s\n", server);

    // Look up the address before we start
    IPAddress address;
    if (!WiFi.hostByName(server, address))
    {
        dbprintf("DNS lookup on %s failed!\n", server);
        return 0;
    }

    if (result_address)
    {
        *result_address = address;
    }

    dbprintf("address: %s\n", address.toString().c_str());

    // wait for the next falling edge of the 1hz square wave
    waitForEdge(SYNC_PIN, PIN_EDGE_FALLING);

    RtcDateTime dt = rtc.GetDateTime();
    EpochTime start_epoch;
    start_epoch.seconds = dt.Epoch32Time();
    start_epoch.fraction = 0;
    OffsetTime offset;
    EpochTime end = ntp.getTime(address, start_epoch, &offset);
    if (end.seconds == 0) {
        dbprintf("NTP Failed!\n");
        return 0;
    }

    if (result_offset != NULL)
    {
        *result_offset = offset;
    }

    uint32_t offset_ms = fraction2Ms(offset.fraction);
    if (abs(offset.seconds) > 0 || offset_ms > 100)
    {
        dbprintf("offset > 100ms, updating RTC!\n");
        uint32_t msdelay = 1000 - offset_ms;
        //dbprintf("msdelay: %u\n", msdelay);

        waitForEdge(SYNC_PIN, PIN_EDGE_FALLING);
        dt = rtc.GetDateTime();

        // wait for where the next second should start
        if (msdelay > 0 && msdelay < 1000)
        {
            delay(msdelay);
        }

        if (sync)
        {
            dt += offset.seconds + 1; // +1 because we waited for the next second
            rtc.SetDateTime(dt);
        }

    }

    return 1;
}

void handleNTP() {
    char server[64];
    strcpy(server, config.ntp_server);
    boolean sync = false;

    if (HTTP.hasArg("server")) {
        dbprintf("SERVER: %s\n", HTTP.arg("server").c_str());
        strncpy(server, HTTP.arg("server").c_str(), 63);
        server[63] = 0;
    }

    if (HTTP.hasArg("sync")) {
        sync = true;
    }

    OffsetTime offset;
    IPAddress address;
    char message[64];
    int code;
    if (setTimeFromNTP(server, sync, &offset, &address))
    {
        if (sync)
        {
            dbprintln("Syncing RTC!");
            syncClockToRTC();
        }
        code = 200;
        uint32_t offset_ms = fraction2Ms(offset.fraction);
        snprintf(message, 64, "OFFSET: %d.%03d (%s)\n", offset.seconds, offset_ms, address.toString().c_str());
    }
    else
    {
        code = 500;
        snprintf(message, 64, "NTP Failed!\n");
    }
    dbprintf(message);
    HTTP.send(code, "text/Plain", message);
    return;
}

void initRTC() {
    dbprint("starting RTC\n");
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

    // don't need the 32khz pin active
#ifdef DS3231
    rtc.Enable32kHzPin(false);
#endif

    //dbprintln("starting 1hz square wave");
#ifdef DS3231
    rtc.SetSquareWavePinClockFrequency(DS3231SquareWaveClock_1Hz);
    rtc.SetSquareWavePin(DS3231SquareWavePin_ModeClock);
#endif
#ifdef DS1307
    rtc.SetSquareWavePin(DS1307SquareWaveOut_1Hz);
#endif
}

void initWiFi()
{
    dbprint("starting wifi feedback\n");
    char tz_offset_str[32];
    char tp_duration_str[8];
    char ap_duration_str[8];
    char ap_delay_str[8];
    char sleep_duration_str[12];
    sprintf(tz_offset_str,      "%d", config.tz_offset);
    sprintf(tp_duration_str,    "%u", config.tp_duration);
    sprintf(ap_duration_str,    "%u", config.ap_duration);
    sprintf(ap_delay_str,       "%u", config.ap_delay);
    sprintf(sleep_duration_str, "%u", config.sleep_duration);

    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);
    dbprintln("create wifi manager");
    WiFiManagerParameter seconds_offset_setting("offset", "Time Zone", tz_offset_str, 32);
    wifi.addParameter(&seconds_offset_setting);
    WiFiManagerParameter position_setting("position", "Clock Position", "", 32);
    wifi.addParameter(&position_setting);
    WiFiManagerParameter ntp_server_setting("ntp_server", "NTP Server", config.ntp_server, 32);
    wifi.addParameter(&ntp_server_setting);
    WiFiManagerParameter warning_label("<p>Advanced Settings!</p>");
    wifi.addParameter(&warning_label);
    WiFiManagerParameter stay_awake_setting("stay_awake", "Stay Awake 'true'", "", 32);
    wifi.addParameter(&stay_awake_setting);
    WiFiManagerParameter sleep_duration_setting("sleep_duration", "Sleep", sleep_duration_str, 32);
    wifi.addParameter(&sleep_duration_setting);
    WiFiManagerParameter tp_duration_setting("tp_duration", "Tick Pulse", tp_duration_str, 32);
    wifi.addParameter(&tp_duration_setting);
    WiFiManagerParameter ap_duration_setting("ap_duration", "Adjust Pulse", ap_duration_str, 32);
    wifi.addParameter(&ap_duration_setting);
    WiFiManagerParameter ap_delay_setting("ap_delay", "Adjust Delay", ap_delay_str, 32);
    wifi.addParameter(&ap_delay_setting);
    wifi.setSaveConfigCallback([](){save_config = true;});
    wifi.setAPCallback([](WiFiManager *){feedback.blink(FEEDBACK_LED_FAST);});
    String ssid = "SynchroClock" + String(ESP.getChipId());
    dbflush();
    if (force_config)
    {
        wifi.startConfigPortal(ssid.c_str(), NULL);
    }
    else
    {
        wifi.autoConnect(ssid.c_str(), NULL);
    }
    feedback.off();

    //
    //  Config was set from captive portal!
    //
    if (save_config)
    {
        int i;
        //
        // update any clock config changes
        //

        dbprintf("clock settings: tp:%s ap:%s,%s\n", tp_duration_setting.getValue(), ap_duration_setting.getValue(), ap_delay_setting.getValue());

        i = atoi(tp_duration_setting.getValue());
        if (i != config.tp_duration)
        {
            config.tp_duration = i;
            clock.setTPDuration(config.tp_duration);
        }

        i = atoi(ap_duration_setting.getValue());
        if (i != config.ap_duration)
        {
            config.ap_duration = i;
            clock.setAPDuration(config.ap_duration);
        }

        i = atoi(ap_delay_setting.getValue());
        if (i != config.ap_delay)
        {
            config.ap_delay = i;
            clock.setAPDelay(config.ap_delay);
        }

        i = atoi(sleep_duration_setting.getValue());
        if (i != config.sleep_duration)
        {
            config.sleep_duration = i;
        }

        dbprintf("ntp setting: %s\n", ntp_server_setting.getValue());
        strncpy(config.ntp_server, ntp_server_setting.getValue(), sizeof(config.ntp_server)-1);

        dbprintf("seconds_offset_setting: %s\n", seconds_offset_setting.getValue());
        const char* seconds_offset_value = seconds_offset_setting.getValue();
        config.tz_offset = parseOffset(seconds_offset_value);

        const char* position_value = position_setting.getValue();
        if (strlen(position_value))
        {
            uint16_t position = parsePosition(position_value);
            dbprintf("setting position to %d\n", position);
            clock.setPosition(position);
        }

        if (strcmp(stay_awake_setting.getValue(), "true") == 0 || strcmp(stay_awake_setting.getValue(), "True") == 0)
        {
            stay_awake = true;
        }

        saveConfig();
    }
}


void setup()
{
    dbbegin(115200);
    dbprintln("");
    dbprintln("Startup!");

    pinMode(SYNC_PIN, INPUT);
    pinMode(FACTORY_RESET_PIN, INPUT);

    //
    // initialize the RTC - note this starts Wire (i2c) as well.
    initRTC();

    //
    // initialize config to defaults then load.
    memset(&config, 0, sizeof(config));
    config.sleep_duration = DEFAULT_SLEEP_DURATION;
    config.tz_offset   = 0;
    config.tp_duration = clock.getTPDuration();
    config.ap_duration = clock.getAPDuration();
    config.ap_delay    = clock.getAPDelay();
    strncpy(config.ntp_server, DEFAULT_NTP_SERVER, sizeof(config.ntp_server)-1);
    config.ntp_server[sizeof(config.ntp_server)-1] = 0;

    dbprintf("defaults: tz:%d tp:%u,%u ap:%u ntp:%s\n", config.tz_offset, config.tp_duration, config.ap_duration, config.ap_delay, config.ntp_server);

    EEPROM.begin(sizeof(EEConfig));
    delay(100);
    // if the saved config was not good then force config.
    if (!loadConfig())
    {
        force_config = true;
    }

    dbprintf("config: tz:%d tp:%u,%u ap:%u ntp:%s\n", config.tz_offset, config.tp_duration, config.ap_duration, config.ap_delay, config.ntp_server);

    //
    // clock parameters could have changed, set them
    clock.setTPDuration(config.tp_duration);
    clock.setAPDuration(config.ap_duration);
    clock.setAPDelay(config.ap_delay);

    // if the config button is pressed then force config
    if (digitalRead(FACTORY_RESET_PIN) == 0)
    {
        force_config = true;
    }

    boolean enabled = clock.getEnable();
    dbprintln("clock enable is:" + String(enabled));

    //
    // if the clock is not running advance it to sync tick/tock
    //
    if (!enabled) {
        clock.setAdjustment(1); // we don't know the initial state of the clock so tick once to sync tic/tock state
        force_config = true;    // force the config portal to set the position if the clock is not running
    }

    initWiFi();

    ntp.begin(1235);
    delay(1000);

    if (!enabled)
    {
        dbprintln("enabling clock");
        clock.setEnable(true);
    }
    else
    {
        dbprintln("clock is enabled, skipping init of RTC");
    }

    dbprintln("syncing RTC from NTP!");
    setTimeFromNTP(config.ntp_server, true, NULL, NULL);

    dbprintln("syncing clock to RTC!");
    syncClockToRTC();

    if (stay_awake)
    {
        dbprintln("starting HTTP");
        HTTP.on("/offset", HTTP_GET, handleOffset);
        HTTP.on("/adjust", HTTP_GET, handleAdjustment);
        HTTP.on("/position", HTTP_GET, handlePosition);
        HTTP.on("/tp_duration", HTTP_GET, handleTPDuration);
        HTTP.on("/ap_duration", HTTP_GET, handleAPDuration);
        HTTP.on("/ap_delay", HTTP_GET, handleAPDelay);
        HTTP.on("/enable", HTTP_GET, handleEnable);
        HTTP.on("/rtc", HTTP_GET, handleRTC);
        HTTP.on("/ntp", HTTP_GET, handleNTP);
        HTTP.begin();
    }
    else
    {
        dbprintf("Deep Sleep Time: %d\n", config.sleep_duration);
        dbflush();
        ESP.deepSleep(config.sleep_duration * 1000000);
    }
}


void loop()
{
    if (stay_awake)
    {
        HTTP.handleClient();
    }
    delay(100);
}

void syncClockToRTC()
{
    // if there is already an adjustment in progress then stop it.
    if (clock.getAdjustment() > 0)
    {
        clock.setAdjustment(0);
    }
    waitForEdge(SYNC_PIN, PIN_EDGE_RISING);
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
    int signed_position = hour * 60 * 60 + minute * 60 + second;
    dbprintf("position before offset: %d\n", signed_position);
    signed_position += config.tz_offset;
    dbprintf("position after offset: %d\n", signed_position);
    if (signed_position < 0) {
        signed_position += MAX_SECONDS;
        dbprintf("position corrected: %d\n", signed_position);
    }
    else if (signed_position >= MAX_SECONDS) {
        signed_position -= MAX_SECONDS;
        dbprintf("position corrected: %d\n", signed_position);
    }
    uint16_t position = (uint16_t)signed_position;
    return position;
}

//
// Wait for an rising or falling edge of the given pin
//
void waitForEdge(int pin, int edge) {
    while (digitalRead(pin) == edge) {
        delay(1);
    }
    while (digitalRead(pin) != edge) {
        delay(1);
    }
}


uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

boolean loadConfig() {
    EEConfig cfg;
    // Read struct from EEPROM
    dbprintln("loading config from EEPROM");
    uint8_t i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        p[i] = EEPROM.read(i);
    }

    uint32_t crcOfData = calculateCRC32(((uint8_t*) &cfg.data), sizeof(cfg.data));
    dbprintf("CRC32 of data: %08x\n", crcOfData);
    dbprintf("CRC32 read from EEPROM: %08x\n", cfg.crc);
    if (crcOfData != cfg.crc)
    {
        dbprintln("CRC32 in EEPROM memory doesn't match CRC32 of data. Data is probably invalid!");
        return false;
    }
    Serial.println("CRC32 check ok, data is probably valid.");
    memcpy(&config, &cfg.data, sizeof(config));
    return true;
}

void saveConfig() {
    EEConfig cfg;
    memcpy(&cfg.data, &config, sizeof(cfg.data));
    cfg.crc = calculateCRC32(((uint8_t*) &cfg.data), sizeof(cfg.data));
    dbprintf("caculated CRC: %08x\n", cfg.crc);
    dbprintln("Saving config to EEPROM");

    uint8_t i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        EEPROM.write(i, p[i]);
    }
    EEPROM.commit();
}
