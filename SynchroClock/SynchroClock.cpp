#include "SynchroClock.h"

Config           config;
DeepSleepData    dsd;
FeedbackLED      feedback(LED_PIN);
ESP8266WebServer HTTP(80);
SNTP             ntp("pool.ntp.org", 123);
Clock            clk(SYNC_PIN);
DS3231           rtc;

boolean save_config  = false; // used by wifi manager when settings were updated.
boolean force_config = false; // reset handler sets this to force into config if btn held
boolean stay_awake   = false; // don't use deep sleep

char message[128]; // buffer for http return values

#ifdef DEBUG_SYNCHRO_CLOCK
unsigned int snprintf(char*, unsigned int, ...);
#define DBP_BUF_SIZE 256
#define dbbegin(x)    {logger.begin(x);logger.setNetworkLogger(NETWORK_LOGGER_HOST, NETWORK_LOGGER_PORT);}
#define dbend()       logger.end()
#define dbprintf(...) logger.printf(__VA_ARGS__)
#define dbprintln(x)  logger.println(x)
#define dbflush()     logger.flush()
#else
#define dbbegin(x)
#define dbend()
#define dbprintf(...)
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

        if (value[0] == '-')
        {
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
        dbprintf("invalid value for %s: %s using 32 instead!", name.c_str(), HTTP.arg(name).c_str());
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
        dbprintf("seconds offset:%d\n", config.tz_offset);
        saveConfig();
    }

    HTTP.send(200, "text/plain", String(config.tz_offset) + "\n");
}

void handleAdjustment()
{
    uint16_t adj;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        if (HTTP.arg("set").equalsIgnoreCase("auto"))
        {
            dbprintln("Auto Adjust!");
            setCLKfromRTC();
        }
        else
        {
            adj = getValidPosition("set");
            dbprintf("setting adjustment:%u\n", adj);
            if (clk.writeAdjustment(adj))
            {
            	dbprintln("failed to set adjustment!");
            }
        }
    }

    if (clk.readAdjustment(&adj))
    {
    	sprintf(message, "failed to read adjustment!\n");
    }
    else
    {
        sprintf(message, "adjustment: %d\n", adj);
    }

    clk.setStayActive(false);

    HTTP.send(200, "text/plain", message);
}

void handlePosition()
{
    uint16_t pos;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        pos = getValidPosition("set");
        dbprintf("setting position:%u\n",pos);
        if (clk.writePosition(pos))
        {
        	dbprintln("failed to set position!");
        }
    }

    if (clk.readPosition(&pos))
    {
    	sprintf(message, "failed to read position!\n");
    }
    else
    {
        int hours = pos / 3600;
        int minutes = (pos - (hours * 3600)) / 60;
        int seconds = pos - (hours * 3600) - (minutes * 60);
        sprintf(message, "position: %d (%02d:%02d:%02d)\n",
                pos, hours,     minutes,     seconds);
    }

    clk.setStayActive(false);

    HTTP.send(200, "text/Plain", message);
}

void handleTPDuration()
{
    uint8_t value;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprintf("setting tp_duration:%u\n", value);
        if (clk.writeTPDuration(value))
        {
        	dbprintln("failed to set TP duration!");
        }
    }

    if (clk.readTPDuration(&value))
    {
    	sprintf(message, "failed to read TP duration\n");
    }
    else
    {
    	sprintf(message, "TP duration: %u\n", value);
    }
    clk.setStayActive(false);

    HTTP.send(200, "text/plain", message);
}

void handleAPDuration()
{
    uint8_t value;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprintf("setting ap_duration:%u\n", value);
        if (clk.writeAPDuration(value))
        {
        	dbprintln("failed to set AP duration");
        }
    }

    if (clk.readAPDuration(&value))
    {
    	sprintf(message, "failed to read AP duration\n");
    }
    else
    {
    	sprintf(message, "AP duration: %u\n", value);
    }
    clk.setStayActive(false);

    HTTP.send(200, "text/plain", message);
}

void handleAPDelay()
{
    uint8_t value;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprintf("setting ap_delay:%u\n", value);
        if (clk.writeAPDelay(value))
        {
        	dbprintln("failed to set AP delay");
        }
    }

    if (clk.readAPDelay(&value))
    {
    	sprintf(message, "failed to read AP delay\n");
    }
    else
    {
    	sprintf(message, "AP delay: %u\n", value);
    }
    clk.setStayActive(false);

    HTTP.send(200, "text/plain", message);
}

void handleSleepDelay()
{
    uint8_t value;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprintf("setting sleep_delay:%u\n", value);
        if (clk.writeSleepDelay(value))
        {
        	dbprintln("failed to set sleep delay");
        }
    }

    if (clk.readSleepDelay(&value))
    {
    	sprintf(message, "failed to read sleep delay\n");
    }
    else
    {
    	sprintf(message, "sleep delay: %u\n", value);
    }
    clk.setStayActive(false);

    HTTP.send(200, "text/plain", String(value) + "\n");
}

void handleEnable()
{
    boolean enable;
    clk.setStayActive(true);
    if (HTTP.hasArg("set"))
    {
        enable = getValidBoolean("set");
        clk.setEnable(enable);
    }
    enable = clk.getEnable();
    clk.setStayActive(false);
    HTTP.send(200, "text/Plain", String(enable) + "\n");
}

void handleRTC()
{
    char message[64];
    DS3231DateTime dt;
    int err = rtc.readTime(dt);

    if (!err)
    {
        dbprintf("handleRTC: RTC : %s (UTC)\n", dt.string());

        clk.setStayActive(true);

        if (HTTP.hasArg("sync"))
        {
            setCLKfromRTC();
        }

        uint16_t value = dt.getPosition(config.tz_offset);

        clk.setStayActive(false);
        int hours = value / 3600;
        int minutes = (value - (hours * 3600)) / 60;
        int seconds = value - (hours * 3600) - (minutes * 60);
        sprintf(message, "%d (%02d:%02d:%02d)\n", value, hours, minutes, seconds);
    }
    else
    {
        sprintf(message, "rtc.readTime() failed!");
    }

    HTTP.send(200, "text/plain", message);
}

void handleNTP()
{
    char server[64];
    strcpy(server, config.ntp_server);
    boolean sync = false;

    if (HTTP.hasArg("server"))
    {
        dbprintf("SERVER: %s\n", HTTP.arg("server").c_str());
        strncpy(server, HTTP.arg("server").c_str(), 63);
        server[63] = 0;
    }

    if (HTTP.hasArg("sync"))
    {
        sync = true;
    }

    OffsetTime offset;
    IPAddress address;
    char message[64];
    int code;
    if (setRTCfromNTP(server, sync, &offset, &address))
    {
        code = 500;
        snprintf(message, 64, "NTP Failed!\n");
    }
    else
    {
        if (sync)
        {
            dbprintln("Syncing clock to RTC!");
            setCLKfromRTC();
        }
        code = 200;
        uint32_t offset_ms = fraction2Ms(offset.fraction);
        snprintf(message, 64, "OFFSET: %d.%03d (%s)\n", offset.seconds, offset_ms, address.toString().c_str());
    }
    dbprintf(message);
    HTTP.send(code, "text/Plain", message);
    return;
}


void handleWire()
{
    char message[64];
    int rtn = WireUtils.clearBus();
    if (rtn != 0)
    {
        snprintf(message, 64, "I2C bus error. Could not clear\n");
        if (rtn == 1)
        {
            snprintf(message, 64, "SCL clock line held low\n");
        }
        else if (rtn == 2)
        {
            snprintf(message, 64, "SCL clock line held low by slave clock stretch\n");
        }
        else if (rtn == 3)
        {
            snprintf(message, 64, "SDA data line held low\n");
        }
    }
    else
    {
        snprintf(message, 64, "Recovered!\n");
    }

    HTTP.send(200, "text/Plain", message);
}

void initWiFi()
{
    dbprintln("starting wifi!");
    char tz_offset_str[32];
    char tp_duration_str[8];
    char ap_duration_str[8];
    char ap_delay_str[8];
    char sleep_delay_str[8];
    char sleep_duration_str[12];
    sprintf(tz_offset_str, "%d", config.tz_offset);
    sprintf(tp_duration_str, "%u", config.tp_duration);
    sprintf(ap_duration_str, "%u", config.ap_duration);
    sprintf(ap_delay_str, "%u", config.ap_delay);
    sprintf(sleep_delay_str, "%u", config.sleep_delay);
    sprintf(sleep_duration_str, "%u", config.sleep_duration);

    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);

    WiFiManager      wifi(20);
    wifi.setDebugOutput(false);

    // TODO: add timezone support

    WiFiManagerParameter seconds_offset_setting("offset", "Time Zone", tz_offset_str, 7);
    wifi.addParameter(&seconds_offset_setting);
    WiFiManagerParameter position_setting("position", "Clock Position", "", 7);
    wifi.addParameter(&position_setting);
    WiFiManagerParameter ntp_server_setting("ntp_server", "NTP Server", config.ntp_server, 32);
    wifi.addParameter(&ntp_server_setting);
    WiFiManagerParameter warning_label("<p>Advanced Settings!</p>");
    wifi.addParameter(&warning_label);
    WiFiManagerParameter stay_awake_setting("stay_awake", "Stay Awake 'true'", "", 7);
    wifi.addParameter(&stay_awake_setting);
    WiFiManagerParameter sleep_duration_setting("sleep_duration", "Sleep", sleep_duration_str, 7);
    wifi.addParameter(&sleep_duration_setting);
    WiFiManagerParameter tp_duration_setting("tp_duration", "Tick Pulse", tp_duration_str, 3);
    wifi.addParameter(&tp_duration_setting);
    WiFiManagerParameter ap_duration_setting("ap_duration", "Adjust Pulse", ap_duration_str, 3);
    wifi.addParameter(&ap_duration_setting);
    WiFiManagerParameter ap_delay_setting("ap_delay", "Adjust Delay", ap_delay_str, 3);
    wifi.addParameter(&ap_delay_setting);
    WiFiManagerParameter sleep_delay_setting("sleep_delay", "Sleep Delay", sleep_delay_str, 4);
    wifi.addParameter(&sleep_delay_setting);

    wifi.setConnectTimeout(CONNECTION_TIMEOUT);

    wifi.setSaveConfigCallback([]()
    {   save_config = true;});
    wifi.setAPCallback([](WiFiManager *)
    {   feedback.blink(FEEDBACK_LED_FAST);});
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
    // if we are not connected then deep sleep and try again.
    if (!WiFi.isConnected())
    {
        dbprintf("failed to connect to wifi!");
        dbprintf("Deep Sleep Time: %d\n", MAX_SLEEP_DURATION);
        dbflush();
        dbend();
        ESP.deepSleep(MAX_SLEEP_DURATION, RF_DEFAULT);
    }

    //
    //  Config was set from captive portal!
    //
    if (save_config)
    {
        int i;

        //
        // update any clock config changes
        //
        dbprintf("clock settings: tp:%s ap:%s,%s slpdrv:%s\n", tp_duration_setting.getValue(),
                ap_duration_setting.getValue(), ap_delay_setting.getValue(), sleep_delay_setting.getValue());

        i = atoi(tp_duration_setting.getValue());
        if (i != config.tp_duration)
        {
            config.tp_duration = i;
            clk.writeTPDuration(config.tp_duration);
        }

        i = atoi(ap_duration_setting.getValue());
        if (i != config.ap_duration)
        {
            config.ap_duration = i;
            clk.writeAPDuration(config.ap_duration);
        }

        i = atoi(ap_delay_setting.getValue());
        if (i != config.ap_delay)
        {
            config.ap_delay = i;
            clk.writeAPDelay(config.ap_delay);
        }

        i = atoi(sleep_delay_setting.getValue());
        if (i != config.sleep_delay)
        {
            config.sleep_delay = i;
            clk.writeSleepDelay(config.sleep_delay);
        }

        uint32_t ul = atol(sleep_duration_setting.getValue());
        if (ul != config.sleep_duration)
        {
            config.sleep_duration = ul;
        }

        dbprintf("ntp setting: %s\n", ntp_server_setting.getValue());
        strncpy(config.ntp_server, ntp_server_setting.getValue(), sizeof(config.ntp_server) - 1);

        dbprintf("seconds_offset_setting: %s\n", seconds_offset_setting.getValue());
        const char* seconds_offset_value = seconds_offset_setting.getValue();
        config.tz_offset = parseOffset(seconds_offset_value);

        const char* position_value = position_setting.getValue();
        if (strlen(position_value))
        {
            uint16_t position = parsePosition(position_value);
            dbprintf("setting position to %d\n", position);
            if (clk.writePosition(position))
            {
            	dbprintln("failed to set initial position!");
            }
        }

        if (strcmp(stay_awake_setting.getValue(), "true") == 0 || strcmp(stay_awake_setting.getValue(), "True") == 0)
        {
            stay_awake = true;
        }

        saveConfig();
    }
    IPAddress ip = WiFi.localIP();
    dbprintf("Connected! IP address is: %u.%u.%u.%u\n",ip[0],ip[1],ip[2],ip[3]);
}

void setup()
{
    dbbegin(115200);
    dbprintln("");
    dbprintln("Startup!");

    uint32_t sleep_duration = MAX_SLEEP_DURATION;
    RFMode mode = RF_DEFAULT;

    pinMode(SYNC_PIN, INPUT);
    pinMode(FACTORY_RESET_PIN, INPUT);
	feedback.off();

	//
	// lets read deep sleep data and see if we need to immed go back to sleep.
	//
	dsd.sleep_delay_left = 0; // this was a cold boot unless RTC data is valid
	readDeepSleepData();

    Wire.begin();

    while (rtc.begin())
    {
    	dbprintln("RTC begin failed! Attempting recovery...");

    	while (WireUtils.clearBus())
    	{
    		delay(10000);
    		dbprintln("lets try that again...");
    	}
		delay(1000);
    }

    DS3231DateTime dt;
    while (rtc.readTime(dt))
    {
    	dbprintln("read RTC failed! Attempting recovery...");
    	//
    	//
    	while(WireUtils.clearBus())
    	{
    		delay(10000);
    		dbprintln("lets try that again...");
    	}
    	delay(10000);
    }
    dbprintf("intial RTC: %s\n", dt.string());

    //
    // initialize config to defaults then load.
    memset(&config, 0, sizeof(config));
    config.sleep_duration = DEFAULT_SLEEP_DURATION;
    config.tz_offset = 0;

    //
    // make sure the device is available!
    //
    feedback.blink(0.9);
    dbprintln("starting clock interface");
    while(clk.begin())
    {
        dbprintln("can't talk with Clock Controller!");
    	while(WireUtils.clearBus())
    	{
    		delay(10000);
    		dbprintln("lets try that again...");
    	}
    	delay(10000);
    }
    dbprintln("clock interface started");
    feedback.off();


    clk.setStayActive(true);

    uint8_t value;
    config.tp_duration = clk.readTPDuration(&value) ? DEFAULT_TP_DURATION : value;
    config.ap_duration = clk.readAPDuration(&value) ? DEFAULT_AP_DURATION : value;
    config.ap_delay    = clk.readAPDelay(&value)    ? DEFAULT_AP_DELAY    : value;
    config.sleep_delay = clk.readSleepDelay(&value) ? DEFAULT_SLEEP_DELAY : value;
    strncpy(config.ntp_server, DEFAULT_NTP_SERVER, sizeof(config.ntp_server) - 1);
    config.ntp_server[sizeof(config.ntp_server) - 1] = 0;

    dbprintf("defaults: tz:%d tp:%u,%u ap:%u ntp:%s\n", config.tz_offset, config.tp_duration, config.ap_duration,
            config.ap_delay, config.ntp_server);

    EEPROM.begin(sizeof(EEConfig));
    delay(100);
    // if the saved config was not good then force config.
    if (!loadConfig())
    {
        force_config = true;
    }

    dbprintf("config: tz:%d tp:%u,%u ap:%u ntp:%s\n", config.tz_offset, config.tp_duration, config.ap_duration,
            config.ap_delay, config.ntp_server);

    // TODO: clean this up, make configurable!
    // DST Start
    config.tz[0].occurrence   = 2;      // second
    config.tz[0].day_of_week = 0;      // Sunday
    config.tz[0].month       = 3;      // of March
    config.tz[0].hour        = 2;      // 2am
    config.tz[0].tz_offset   = -25200; // UTC - 7 Hours
    // DST End
    config.tz[1].occurrence  = 1;      // first
    config.tz[1].day_of_week = 0;      // Sunday
    config.tz[1].month       = 11;     // of November
    config.tz[1].hour        = 2;      // 2am
    config.tz[1].tz_offset   = -28800; // UTC - 8 Hours

    dt.applyOffset(config.tz_offset);
    int new_offset = TZUtils::computeCurrentTZOffset(dt, config.tz, TZ_COUNT);

    // if the time zone changed then save the new value and set the the clock
    if (config.tz_offset != new_offset)
    {
        dbprintf("time zone offset changed from %d to %d\n", config.tz_offset, new_offset);
        config.tz_offset = new_offset;
        saveConfig();
        setCLKfromRTC();
    }

    // if the config button is pressed then force config
    if (digitalRead(FACTORY_RESET_PIN) == 0)
    {
        //
        // If we wake with the reset button pressed and sleep_delay_left then the radio is off
        // so set it to 0 and use a very short deepSleep to turn the radio back on.
        //
        if (dsd.sleep_delay_left != 0)
        {
            clk.setStayActive(false);
            dbprintln("reset button pressed with radio off, short sleep to enable!");
            dsd.sleep_delay_left = 0;
            writeDeepSleepData();
            ESP.deepSleep(1, RF_DEFAULT); // super short sleep to enable the radio!
        }
        dbprintln("reset button pressed, forcing config!");
        force_config = true;
    }

    dbprintf("sleep_delay_left: %lu\n", dsd.sleep_delay_left);

    if (dsd.sleep_delay_left != 0)
    {

        if (dsd.sleep_delay_left > MAX_SLEEP_DURATION)
        {
            dsd.sleep_delay_left = dsd.sleep_delay_left - MAX_SLEEP_DURATION;
            mode = RF_DISABLED;
            dbprintf("delay still greater than max, mode=DISABLED sleep_delay_left=%lu\n", dsd.sleep_delay_left);
        }
        else
        {
            sleep_duration       = dsd.sleep_delay_left;
            dsd.sleep_delay_left = 0;
            dbprintf("delay less than max, mode=DEFAULT sleep_delay_left=%lu\n", dsd.sleep_delay_left);
        }

        clk.setStayActive(false);
        writeDeepSleepData();
        dbprintf("Deep Sleep Time: %lu\n", sleep_duration);
        dbflush();
        dbend();
        ESP.deepSleep(sleep_duration * 1000000, mode);
    }

    //
    // clock parameters could have changed, set them
    clk.writeTPDuration(config.tp_duration);
    clk.writeAPDuration(config.ap_duration);
    clk.writeAPDelay(config.ap_delay);

    boolean enabled = clk.getEnable();
    dbprintf("clock enable is:%u\n",enabled);

    //
    // if the clock is not running advance it to sync tick/tock
    //
#ifndef DISABLE_DEEP_SLEEP
    if (!enabled)
    {
        force_config = true;    // force the config portal to set the position if the clock is not running
    }
#endif

    initWiFi();

    ntp.begin(1235);

    if (!enabled)
    {
        dbprintln("enabling clock");
        clk.setEnable(true);
    }
    else
    {
        dbprintln("clock is enabled, skipping init of RTC");
    }

#ifndef DISABLE_INITIAL_NTP
    dbprintln("syncing RTC from NTP!");
    setRTCfromNTP(config.ntp_server, true, NULL, NULL);
#endif
#ifndef DISABLE_INITIAL_SYNC
    dbprintln("syncing clock to RTC!");
    setCLKfromRTC();
#endif

    clk.setStayActive(false);

#ifdef DISABLE_DEEP_SLEEP
    stay_awake = true;
#endif

    if (stay_awake)
    {
        dbprintln("starting HTTP");
        HTTP.on("/offset", HTTP_GET, handleOffset);
        HTTP.on("/adjust", HTTP_GET, handleAdjustment);
        HTTP.on("/position", HTTP_GET, handlePosition);
        HTTP.on("/tp_duration", HTTP_GET, handleTPDuration);
        HTTP.on("/ap_duration", HTTP_GET, handleAPDuration);
        HTTP.on("/ap_delay", HTTP_GET, handleAPDelay);
        HTTP.on("/sleep_delay", HTTP_GET, handleSleepDelay);
        HTTP.on("/enable", HTTP_GET, handleEnable);
        HTTP.on("/rtc", HTTP_GET, handleRTC);
        HTTP.on("/ntp", HTTP_GET, handleNTP);
        HTTP.on("/wire", HTTP_GET, handleWire);
        HTTP.begin();
    }
    else
    {
        dsd.sleep_delay_left = 0;
        sleep_duration       = config.sleep_duration;
        mode                 = RF_DEFAULT;
        if (sleep_duration > MAX_SLEEP_DURATION)
        {
            dsd.sleep_delay_left = sleep_duration - MAX_SLEEP_DURATION;
            sleep_duration       = MAX_SLEEP_DURATION;
            mode                 = RF_DISABLED;
            dbprintf("sleep_duration > max, mode=DISABLE, sleep_delay_left = %lu\n", dsd.sleep_delay_left);
        }
        writeDeepSleepData();
        dbprintf("Deep Sleep Time: %lu\n", sleep_duration);
        dbflush();
        dbend();
        ESP.deepSleep(sleep_duration * 1000000, mode);
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

int setRTCfromNTP(const char* server, bool sync, OffsetTime* result_offset, IPAddress* result_address)
{
    dbprintf("using server: %s\n", server);

    // Look up the address before we start
    IPAddress address;
    if (!WiFi.hostByName(server, address))
    {
        dbprintf("DNS lookup on %s failed!\n", server);
        return ERROR_DNS;
    }

    if (result_address)
    {
        *result_address = address;
    }

    dbprintf("address: %s\n", address.toString().c_str());

    // wait for the next falling edge of the 1hz square wave
    clk.waitForEdge(CLOCK_EDGE_FALLING);

    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dbprintln("setRTCfromNTP: failed to read from RTC!");
        return ERROR_RTC;
    }

    dbprintf("RTC: %s (UTC)\n", dt.string());

    EpochTime start_epoch;
    start_epoch.seconds = dt.getUnixTime();

    dbprintf("start unix: %u\n", start_epoch.seconds);

    start_epoch.fraction = 0;
    OffsetTime offset;
    EpochTime end = ntp.getTime(address, start_epoch, &offset);
    if (end.seconds == 0)
    {
        dbprintf("NTP Failed!\n");
        return ERROR_NTP;
    }

    if (result_offset != NULL)
    {
        *result_offset = offset;
    }

    uint32_t offset_ms = fraction2Ms(offset.fraction);
    dbprintf("NTP req: %s offset: %d.%03u\n",
            dt.string(),
            offset.seconds,
            offset_ms);

    if (abs(offset.seconds) > 0 || offset_ms > 100)
    {
        dbprintf("offset > 100ms, updating RTC!\n");
        uint32_t msdelay = 1000 - offset_ms;
        //dbprintf("msdelay: %u\n", msdelay);

        clk.waitForEdge(CLOCK_EDGE_FALLING);
        if (rtc.readTime(dt))
        {
            dbprintln("setRTCfromNTP: failed to read from RTC!");
            return ERROR_RTC;
        }

        // wait for where the next second should start
        if (msdelay > 0 && msdelay < 1000)
        {
            delay(msdelay);
        }

        if (sync)
        {
            dt.setUnixTime(dt.getUnixTime()+offset.seconds+1); // +1 because we waited for the next second
            if (rtc.writeTime(dt))
            {
                dbprintf("FAILED to set RTC: %s\n", dt.string());
                return ERROR_RTC;
            }
            dbprintf("set RTC: %s\n", dt.string());
        }

    }

    return 0;
}

int setCLKfromRTC()
{
    // if there is already an adjustment in progress then stop it.
    if (clk.writeAdjustment(0))
    {
    	dbprintln("setCLKfromRTC: failed to clear adjustment!");
    	return -1;
    }

    clk.waitForEdge(CLOCK_EDGE_FALLING);
    delay(10);
    uint16_t clock_pos;
    if (clk.readPosition(&clock_pos))
    {
    	dbprintln("setCLKfromRTC: failed to read position, ignoring");
    	return -1;
    }
    dbprintf("setCLKfromRTC: clock position:%d\n", clock_pos);

    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dbprintln("setCLKfromRTC: FAILED to read RTC");
        return -1;
    }
    uint16_t rtc_pos = dt.getPosition(config.tz_offset);
    dbprintf("setCLKfromRTC: RTC position:%d\n", rtc_pos);

    if (clock_pos != rtc_pos)
    {
        int adj = rtc_pos - clock_pos;
        if (adj < 0)
        {
            adj += MAX_POSITION;
        }
        dbprintf("setCLKfromRTC: sending adjustment of %u\n", adj);
        clk.waitForEdge(CLOCK_EDGE_RISING);
        if (clk.writeAdjustment(adj))
        {
        	dbprintln("setCLKfromRTC: failed to set adjustment!");
        	return -1;
        }
    }

    return 0;
}

uint32_t calculateCRC32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xffffffff;
    while (length--)
    {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1)
        {
            bool bit = crc & 0x80000000;
            if (c & i)
            {
                bit = !bit;
            }
            crc <<= 1;
            if (bit)
            {
                crc ^= 0x04c11db7;
            }
        }
    }
    return crc;
}

boolean loadConfig()
{
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
    //dbprintf("CRC32 of data: %08x\n", crcOfData);
    //dbprintf("CRC32 read from EEPROM: %08x\n", cfg.crc);
    if (crcOfData != cfg.crc)
    {
        dbprintln("CRC32 in EEPROM memory doesn't match CRC32 of data. Data is probably invalid!");
        return false;
    }
    //dbprintln("CRC32 check ok, data is probably valid.");
    memcpy(&config, &cfg.data, sizeof(config));
    return true;
}

void saveConfig()
{
    EEConfig cfg;
    memcpy(&cfg.data, &config, sizeof(cfg.data));
    cfg.crc = calculateCRC32(((uint8_t*) &cfg.data), sizeof(cfg.data));
    //dbprintf("caculated CRC: %08x\n", cfg.crc);
    //dbprintln("Saving config to EEPROM");

    uint8_t i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        EEPROM.write(i, p[i]);
    }
    EEPROM.commit();
}

boolean readDeepSleepData()
{
    RTCDeepSleepData rtcdsd;
    dbprintln("loading deep sleep data from RTC Memory");
    if (!ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcdsd, sizeof(rtcdsd)))
    {
        dbprintln("readDeepSleepData: failed to read RTC Memory");
        return false;
    }

    uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcdsd.data), sizeof(rtcdsd.data));
    if (crcOfData != rtcdsd.crc)
    {
        dbprintln("CRC32 in RTC Memory doesn't match CRC32 of data. Data is probably invalid!");
        return false;
    }
    memcpy(&dsd, &rtcdsd.data, sizeof(dsd));
    return true;
}

boolean writeDeepSleepData()
{
    RTCDeepSleepData rtcdsd;
    memcpy(&rtcdsd.data, &dsd, sizeof(rtcdsd.data));
    rtcdsd.crc = calculateCRC32(((uint8_t*) &rtcdsd.data), sizeof(rtcdsd.data));

    if (!ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcdsd, sizeof(rtcdsd)))
    {
        dbprintln("writeDeepSleepData: failed to write RTC Memory");
        return false;
    }
    return true;
}
