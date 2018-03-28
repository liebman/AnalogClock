/*
 * SynchroClock.cpp
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
 *  Created on: May 26, 2017
 *      Author: liebman
 */

#include "SynchroClock.h"
#include "SynchroClockVersion.h"

//
// These are only used when debugging
//
//#define DISABLE_DEEP_SLEEP
//#define DISABLE_INITIAL_NTP
//#define DISABLE_INITIAL_SYNC

DLog&             dlog = DLog::getLog();
Config           config;                    // configuration persisted in the EEPROM
DeepSleepData    dsd;                       // data persisted in the RTC memory
#if defined(LED_PIN)
FeedbackLED      feedback(LED_PIN);         // used to blink LED to indicate status
#endif
ESP8266WebServer HTTP(80);                  // used when debugging/stay awake mode
NTP              ntp(&(dsd.ntp_runtime), &(config.ntp_persist), &saveConfig);   // handles NTP communication & filtering
Clock            clk(SYNC_PIN);             // clock ticker, manages position of clock
DS3231           rtc;                       // real time clock on i2c interface

boolean save_config  = false; // used by wifi manager when settings were updated.
boolean force_config = false; // reset handler sets this to force into config mode if button held
boolean stay_awake   = false; // don't use deep sleep (from config mode option)

const char* ota_url = nullptr; // over-the-air update url
const char* ota_fp  = nullptr; // over-the-air update fingerprint

char message[128]; // buffer for http return values

bool isOTASet()
{
    return ota_url != nullptr;
}

void setOTAurl(const char* value)
{
    if (ota_url != nullptr)
    {
        free((void*)ota_url);
    }
    ota_url = strdup(value);
}

const char* getOTAurl()
{
    return ota_url != nullptr ? ota_url : "";
}

void setOTAfp(const char* value)
{
    if (ota_fp != nullptr)
    {
        free((void*)ota_fp);
    }
    ota_fp = strdup(value);
}

const char* getOTAfp()
{
    return ota_fp != nullptr ? ota_fp : "";
}

boolean parseBoolean(const char* value)
{
    if (!strcmp(value, "true") || !strcmp(value, "True") || !strcmp(value, "TRUE") || atoi(value))
    {
        return true;
    }
    return false;
}

uint8_t parseDuty(const char* value)
{
    int i = atoi(value);
    if (i < 1 || i > 100)
    {
        dlog.error(F("parseDuty"), F("invalid value %s: using 50 instead!"), value);
        i = 50;
    }
    return (uint8_t) i;
}

int getValidOffset(String name)
{
    int result = TimeUtils::parseOffset(HTTP.arg(name).c_str());

    return result;
}

uint16_t getValidPosition(String name)
{
    int result = TimeUtils::parsePosition(HTTP.arg(name).c_str());

    return result;
}

uint8_t getValidDuration(String name)
{
    uint8_t result = TimeUtils::parseSmallDuration(HTTP.arg(name).c_str());
    return result;
}

uint8_t getValidDuty(String name)
{
    uint8_t result = parseDuty(HTTP.arg(name).c_str());
    return result;
}

boolean getValidBoolean(String name)
{
    boolean result = parseBoolean(HTTP.arg(name).c_str());
    return result;
}

uint8_t getValidByte(String name)
{
    int i = atoi(HTTP.arg(name).c_str());
    if (i < 0 || i > 255)
    {
        dlog.error(F("getValidByte"), F("invalid value %d: using 255 instead!"), i);
        i = 255;
    }
    return (uint8_t) i;
}

void handleOffset()
{
    if (HTTP.hasArg("set"))
    {
        config.tz_offset = getValidOffset("set");
        dlog.info(F("handleOffset"), F("seconds offset:%d"), config.tz_offset);
    }

    HTTP.send(200, "text/plain", String(config.tz_offset) + "\n");
}

void handleAdjustment()
{
    static PROGMEM const char TAG[] = "handleAdjustment";
    uint16_t adj;
    if (HTTP.hasArg("set"))
    {
        if (HTTP.arg("set").equalsIgnoreCase("auto"))
        {
            dlog.info(FPSTR(TAG), F("Auto Adjust!"));
            setCLKfromRTC();
        }
        else
        {
            adj = getValidPosition("set");
            dlog.info(FPSTR(TAG), F("setting adjustment:%u"), adj);
            if (clk.writeAdjustment(adj))
            {
                dlog.error(FPSTR(TAG), F("failed to set adjustment!"));
            }
        }
    }

    if (clk.readAdjustment(&adj))
    {
        sprintf_P(message, PSTR("failed to read adjustment!\n"));
    }
    else
    {
        sprintf_P(message, PSTR("adjustment: %d\n"), adj);
    }

    HTTP.send(200, "text/plain", message);
}

void handlePosition()
{
    static PROGMEM const char TAG[] = "handlePosition";

    uint16_t pos;
    if (HTTP.hasArg("set"))
    {
        pos = getValidPosition("set");
        dlog.info(FPSTR(TAG), F("setting position:%u"), pos);
        if (clk.writePosition(pos))
        {
            dlog.error(FPSTR(TAG), F("failed to set position!"));
        }
    }

    if (clk.readPosition(&pos))
    {
        sprintf_P(message, PSTR("failed to read position!\n"));
    }
    else
    {
        int hours = pos / 3600;
        int minutes = (pos - (hours * 3600)) / 60;
        int seconds = pos - (hours * 3600) - (minutes * 60);
        sprintf_P(message, PSTR("position: %d (%02d:%02d:%02d)\n"), pos, hours, minutes, seconds);
    }

    HTTP.send(200, "text/Plain", message);
}

void handleTPDuration()
{
    static PROGMEM const char TAG[] = "handleTPDuration";

    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dlog.info(FPSTR(TAG), F("setting tp_duration:%u"), value);
        if (clk.writeTPDuration(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set TP duration!"));
        }
    }

    if (clk.readTPDuration(&value))
    {
        sprintf_P(message, PSTR("failed to read TP duration\n"));
    }
    else
    {
        sprintf_P(message, PSTR("TP duration: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleTPDuty()
{
    static PROGMEM const char TAG[] = "handleTPDuty";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuty("set");
        dlog.info(FPSTR(TAG), F("setting tp_duty:%u"), value);
        if (clk.writeTPDuty(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set TP duty!"));
        }
    }

    if (clk.readTPDuty(&value))
    {
        sprintf_P(message, PSTR("failed to read TP duty\n"));
    }
    else
    {
        sprintf_P(message, PSTR("TP duty: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPDuration()
{
    static PROGMEM const char TAG[] = "handleAPDuration";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dlog.info(FPSTR(TAG), F("setting ap_duration:%u"), value);
        if (clk.writeAPDuration(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set AP duration"));
        }
    }

    if (clk.readAPDuration(&value))
    {
        sprintf_P(message, PSTR("failed to read AP duration\n"));
    }
    else
    {
        sprintf_P(message, PSTR("AP duration: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPDuty()
{
    static PROGMEM const char TAG[] = "handleAPDuty";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuty("set");
        dlog.info(FPSTR(TAG), F("setting ap_duty:%u"), value);
        if (clk.writeAPDuty(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set AP duty!"));
        }
    }

    if (clk.readAPDuty(&value))
    {
        sprintf_P(message, PSTR("failed to read AP duty\n"));
    }
    else
    {
        sprintf_P(message, PSTR("AP duty: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPDelay()
{
    static PROGMEM const char TAG[] = "handleAPDelay";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dlog.info(FPSTR(TAG), F("setting ap_delay:%u"), value);
        if (clk.writeAPDelay(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set AP delay"));
        }
    }

    if (clk.readAPDelay(&value))
    {
        sprintf_P(message, PSTR("failed to read AP delay\n"));
    }
    else
    {
        sprintf_P(message, PSTR("AP delay: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPStartDuration()
{
    static PROGMEM const char TAG[] = "handleAPStartDuration";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dlog.info(FPSTR(TAG), F("setting ap_start:%u"), value);
        if (clk.writeAPStartDuration(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set AP start duration"));
        }
    }

    if (clk.readAPStartDuration(&value))
    {
        sprintf_P(message, PSTR("failed to read AP start duration\n"));
    }
    else
    {
        sprintf_P(message, PSTR("AP start duration: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handlePWMTop()
{
    static PROGMEM const char TAG[] = "handlePWMTop";
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidByte("set");
        dlog.info(FPSTR(TAG), F("setting pwm_top:%u"), value);
        if (clk.writePWMTop(value))
        {
            dlog.error(FPSTR(TAG), F("failed to set pwm_top!"));
        }
    }

    if (clk.readPWMTop(&value))
    {
        sprintf_P(message, PSTR("failed to read pwm_top!\n"));
    }
    else
    {
        sprintf_P(message, PSTR("pwm_top: %u\n"), value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleEnable()
{
    boolean enable;
    if (HTTP.hasArg("set"))
    {
        enable = getValidBoolean("set");
        clk.setEnable(enable);
    }
    enable = clk.getEnable();
    HTTP.send(200, "text/Plain", String(enable) + "\n");
}

void handleRTC()
{
    static PROGMEM const char TAG[] = "handleRTC";
    char message[64];

    DS3231DateTime dt;
    int err = rtc.readTime(dt);

    if (!err)
    {
        dlog.info(FPSTR(TAG), F("handleRTC: RTC : %s (UTC)"), dt.string());

        if (HTTP.hasArg("offset"))
        {
            double offset = strtod(HTTP.arg("offset").c_str(), NULL);
            dlog.info(FPSTR(TAG), F("handleRTC: offset: %lf"), offset);
            setRTCfromOffset(offset, true);
        }
        else if (HTTP.hasArg("sync") && getValidBoolean("sync"))
        {
            setCLKfromRTC();
        }

        uint16_t value = dt.getPosition(config.tz_offset);

        int hours = value / 3600;
        int minutes = (value - (hours * 3600)) / 60;
        int seconds = value - (hours * 3600) - (minutes * 60);
        sprintf_P(message, PSTR("%d (%02d:%02d:%02d)\n"), value, hours, minutes, seconds);
    }
    else
    {
        sprintf_P(message, PSTR("rtc.readTime() failed!\n"));
    }

    HTTP.send(200, "text/plain", message);
}

void handleNTP()
{
    static PROGMEM const char TAG[] = "handleNTP";
    char server[64];
    strcpy(server, config.ntp_server);
    boolean sync = false;

    if (HTTP.hasArg("server"))
    {
        dlog.info(FPSTR(TAG), F("SERVER: %s"), HTTP.arg("server").c_str());
        strncpy(server, HTTP.arg("server").c_str(), 63);
        server[63] = 0;
    }

    if (HTTP.hasArg("sync"))
    {
        sync = getValidBoolean("sync");
    }

    double offset;
    IPAddress address;
    char message[64];
    int code;
    if (setRTCfromNTP(server, sync, &offset, &address))
    {
        code = 500;
        snprintf_P(message, 64, PSTR("NTP Failed!\n"));
    }
    else
    {
        if (sync)
        {
            dlog.info(FPSTR(TAG), F("Syncing clock to RTC!"));
            setCLKfromRTC();
        }
        code = 200;

        snprintf_P(message, 64, PSTR("OFFSET: %0.6lf (%s)\n"), offset, address.toString().c_str());
    }

    dlog.info(FPSTR(TAG), F("%s"), message);
    HTTP.send(code, "text/Plain", message);
    return;
}

void handleWire()
{
    int rtn = WireUtils.clearBus();
    if (rtn != 0)
    {
        if (rtn == 1)
        {
            snprintf_P(message, 64, PSTR("SCL clock line held low\n"));
        }
        else if (rtn == 2)
        {
            snprintf_P(message, 64, PSTR("SCL clock line held low by slave clock stretch\n"));
        }
        else if (rtn == 3)
        {
            snprintf_P(message, 64, PSTR("SDA data line held low\n"));
        }
        else
        {
            snprintf_P(message, 64, PSTR("I2C bus error. Could not clear\n"));
        }
    }
    else
    {
        snprintf_P(message, 64, PSTR("Recovered!\n"));
    }

    HTTP.send(200, "text/Plain", message);
}

void handleSave()
{
    clk.saveConfig();
    saveConfig();
    HTTP.send(200, "text/plain", "Saved!\n");
}

void handleErase()
{
    eraseConfig();
    HTTP.send(200, "text/plain", "Erased!\n");
}


//
// update the timezone offset based on the current date/time
// return true if offset was updated
//
bool updateTZOffset()
{
    static PROGMEM const char TAG[] = "updateTZOffset";

    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dlog.error(FPSTR(TAG), F("updateTZOffset: FAILED to read RTC"));
        return false;
    }

    int new_offset = TimeUtils::computeUTCOffset(dt.getUnixTime(), config.tz_offset, config.tc, TIME_CHANGE_COUNT);

    // if the time zone changed then save the new value and return true
    if (config.tz_offset != new_offset)
    {
        dlog.info(FPSTR(TAG), F("time zone offset changed from %d to %d"), config.tz_offset, new_offset);
        config.tz_offset = new_offset;
        saveConfig();
        return true;
    }

    return false;
}

void createWiFiParams(WiFiManager& wifi, std::vector<ConfigParamPtr> &params)
{
    static PROGMEM const char TAG[] = "wifiParams";
    uint16_t pos = 0;
    char pos_str[16];
    clk.readPosition(&pos, 3);
    int hours = pos / 3600;
    int minutes = (pos - (hours * 3600)) / 60;
    int seconds = pos - (hours * 3600) - (minutes * 60);
    memset(pos_str, 0, sizeof(pos_str));
    snprintf_P(pos_str, 15, PSTR("%02d:%02d:%02d"), hours, minutes, seconds);

    params.push_back(std::make_shared<ConfigParam>(wifi, "position", "Clock Position", pos_str, 10, [](const char* result)
    {
        if (strlen(result))
        {
            uint16_t position = TimeUtils::parsePosition(result);
            dlog.info(FPSTR(TAG), F("setting position to %d"), position);
            if (clk.writePosition(position))
            {
                dlog.error(FPSTR(TAG), F("failed to set initial position!"));
            }
        }
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "ntp_server", "NTP Server", config.ntp_server, 32, [](const char* result)
    {
            strncpy(config.ntp_server, result, sizeof(config.ntp_server) - 1);
    }));

    params.push_back(std::make_shared<ConfigParam>(wifi, "<p>OTA Update</p>"));
    params.push_back(std::make_shared<ConfigParam>(wifi, "ota_url", "OTA URL", getOTAurl(), 127, [](const char* result)
    {
        setOTAurl(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "ota_fp", "OTA fingerprint", getOTAfp(), 127, [](const char* result)
    {
        setOTAfp(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "<p>1st Time Change</p>"));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_occurrence", "occurrence", config.tc[0].occurrence, 3, [](const char* result)
    {
        config.tc[0].occurrence = TimeUtils::parseOccurrence(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_day_of_week", "Day of Week (Sun=0)", config.tc[0].day_of_week, 2, [](const char* result)
    {
        config.tc[0].day_of_week = TimeUtils::parseDayOfWeek(result);
    }));

    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_day_offset", "day offset", config.tc[0].day_offset, 3, [](const char* result)
    {
        config.tc[0].day_offset = TimeUtils::parseOccurrence(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_month", "Month (Jan=1)", config.tc[0].month, 3, [](const char* result)
    {
        config.tc[0].month = TimeUtils::parseMonth(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_hour", "Hour (0-23)", config.tc[0].hour, 3, [](const char* result)
    {
        config.tc[0].hour = TimeUtils::parseHour(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc1_offset", "Time Offset", config.tc[0].tz_offset, 8, [](const char* result)
    {
        config.tc[0].tz_offset = TimeUtils::parseOffset(result);
    }));

    params.push_back(std::make_shared<ConfigParam>(wifi, "<p>2nd Time Change</p>"));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_occurrence", "occurrence", config.tc[1].occurrence, 3, [](const char* result)
    {
        config.tc[1].occurrence = TimeUtils::parseOccurrence(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_day_of_week", "Day of Week (Sun=0)", config.tc[1].day_of_week, 2, [](const char* result)
    {
        config.tc[1].day_of_week = TimeUtils::parseDayOfWeek(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_day_offset", "day offset", config.tc[1].day_offset, 3, [](const char* result)
    {
        config.tc[1].day_offset = TimeUtils::parseOccurrence(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_month", "Month (Jan=1)", config.tc[1].month, 3, [](const char* result)
    {
        config.tc[1].month = TimeUtils::parseMonth(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_hour", "Hour (0-23)", config.tc[1].hour, 3, [](const char* result)
    {
        config.tc[1].hour = TimeUtils::parseHour(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "tc2_offset", "Time Offset", config.tc[1].tz_offset, 8, [](const char* result)
    {
        config.tc[1].tz_offset = TimeUtils::parseOffset(result);
    }));

    params.push_back(std::make_shared<ConfigParam>(wifi, "<p>Advanced Settings!</p>"));
    params.push_back(std::make_shared<ConfigParam>(wifi, "stay_awake", "Stay Awake 'true'", "", 8, [](const char* result)
    {
        stay_awake = parseBoolean(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "sleep_duration", "Sleep", config.sleep_duration, 8, [](const char* result)
    {
        config.sleep_duration = atoi(result);
    }));
    uint8_t value;
    clk.readTPDuration(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "tp_duration", "Tick Pulse", value, 8, [](const char* result)
    {
        uint8_t tp_duration = TimeUtils::parseSmallDuration(result);
        clk.writeTPDuration(tp_duration);
    }));
    clk.readTPDuty(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "tp_duty", "Tick Pulse Duty", value, 8, [](const char* result)
    {
        uint8_t tp_duty = parseDuty(result);
        clk.writeTPDuty(tp_duty);
    }));
    clk.readAPStartDuration(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "ap_start", "Adjust Start Pulse", value, 4, [](const char* result)
    {
        uint8_t ap_start = TimeUtils::parseSmallDuration(result);
        clk.writeAPStartDuration(ap_start);
    }));
    clk.readAPDuration(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "ap_duration", "Adjust Pulse", value, 4, [](const char* result)
    {
        uint8_t ap_duration = TimeUtils::parseSmallDuration(result);
        clk.writeAPDuration(ap_duration);
    }));
    clk.readAPDuty(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "ap_duty", "Adjust Pulse Duty", value, 8, [](const char* result)
    {
        uint8_t ap_duty = parseDuty(result);
        clk.writeAPDuty(ap_duty);
    }));
    clk.readAPDelay(&value);
    params.push_back(std::make_shared<ConfigParam>(wifi, "ap_delay", "Adjust Delay", value, 4, [](const char* result)
    {
        uint8_t ap_delay = TimeUtils::parseSmallDuration(result);
        clk.writeAPDelay(ap_delay);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "network_logger_host", "Network Log Host", config.network_logger_host, 32, [](const char* result)
    {
        strncpy(config.network_logger_host, result, sizeof(config.network_logger_host) - 1);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "network_logger_port", "Network Log Port", config.network_logger_port, 6, [](const char* result)
    {
        config.network_logger_port = atoi(result);
    }));
    params.push_back(std::make_shared<ConfigParam>(wifi, "clear_ntp_persist", "Clear NTP Persist 'true'", "", 8, [](const char* result)
    {
        boolean clearit = parseBoolean(result);
        if (clearit)
        {
            memset(&config.ntp_persist, 0, sizeof(config.ntp_persist));
        }
    }));
}

void initWiFi()
{
    static PROGMEM const char TAG[] = "initWiFi";
    dlog.info(FPSTR(TAG), F("starting WiFi!"));

    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);

    WiFiManager wm;
    wm.setEnableConfigPortal(false); // don't automatically use the captive portal
    wm.setDebugOutput(false);
    wm.setConnectTimeout(CONNECTION_TIMEOUT);
    wm.setSaveConfigCallback([]()
    {
        save_config = true;
    });

    wm.setAPCallback([](WiFiManager *)
    {
        dlog.info(FPSTR(TAG), F("config portal up!"));
        feedback.blink(FEEDBACK_LED_FAST);
    });

    std::vector<ConfigParamPtr> params;
    createWiFiParams(wm, params);

    dlog.info(FPSTR(TAG), "params has %d items", params.size());

    String ssid = "SynchroClock" + String(ESP.getChipId());

    if (force_config)
    {
        wm.startConfigPortal(ssid.c_str(), NULL);
    }
    else
    {
        wm.autoConnect(ssid.c_str(), NULL);
    }

    feedback.off();

    //
    // if we are not connected then deep sleep and try again.
    if (!WiFi.isConnected())
    {
        dlog.error(FPSTR(TAG), F("failed to connect to wifi!"));
        sleepFor(MAX_SLEEP_DURATION);
    }

    IPAddress ip = WiFi.localIP();
    dlog.info(FPSTR(TAG), F("Connected! IP address is: %u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);

    //
    //  Config was set from captive portal!
    //
    if (save_config)
    {
        dlog.info(FPSTR(TAG), F("updating changed params"));
        for(ConfigParamPtr p : params)
        {
            p->applyIfChanged();
        }

        clk.saveConfig();
        saveConfig();
        updateTZOffset();
    }

    dlog.debug(FPSTR(TAG), F("TCP logging settings: '%s:%d'"), config.network_logger_host, config.network_logger_port);

    // Configure network logging if enabled
    if (strlen(config.network_logger_host) && config.network_logger_port)
    {
        dlog.info(FPSTR(TAG), F("starting TCP logging to '%s:%d'"), config.network_logger_host, config.network_logger_port);
        dlog.begin(new DLogTCPWriter(config.network_logger_host, config.network_logger_port));
    }
}

void processOTA(bool enable_clock)
{
    static PROGMEM const char TAG[] = "processOTA";

    if (isOTASet())
    {
        dlog.info(FPSTR(TAG), F("checking for OTA from: '%s'"), getOTAurl());
        feedback.blink(FEEDBACK_LED_MEDIUM);

        ESPhttpUpdate.rebootOnUpdate(false);

        t_httpUpdate_return ret;

        if (strncmp(getOTAurl(), "https:", 6) == 0)
        {
            ret = ESPhttpUpdate.update(getOTAurl(), SYNCHRO_CLOCK_VERSION, getOTAfp());
        }
        else
        {
            ret = ESPhttpUpdate.update(getOTAurl(), SYNCHRO_CLOCK_VERSION);
        }

        String reason = ESPhttpUpdate.getLastErrorString();

        switch(ret)
        {
            case HTTP_UPDATE_OK:
                clk.setEnable(enable_clock);
                dlog.info(FPSTR(TAG), F("OTA update OK! restarting..."));
                dlog.end();
                ESP.restart();
                break;

            case HTTP_UPDATE_FAILED:
                dlog.info(FPSTR(TAG), F("OTA update failed! reason:'%s'"), reason.c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                dlog.info(FPSTR(TAG), F("OTA no updates! reason:'%s'"), reason.c_str());
                break;

            default:
                dlog.info(FPSTR(TAG), F("OTA update WTF? unexpected return code: %d reason: '%s'"), ret, reason.c_str());
                break;
        }
    }
}

void factoryReset()
{
    static PROGMEM const char TAG[] = "factoryReset";

    dlog.info(FPSTR(TAG), F("activated!"));

    dlog.info(FPSTR(TAG), F("sending factory reset to Clock..."));
    feedback.blink(FEEDBACK_LED_SLOW);
    while (clk.factoryReset() != 0)
    {
        dlog.error(FPSTR(TAG), F("can't talk with Clock Controller!"));
        while (WireUtils.clearBus())
        {
            delay(10000);
            dlog.info(FPSTR(TAG), F("lets try that again..."));
        }
        delay(10000);

    }
    feedback.off();

    dlog.info(FPSTR(TAG), F("invalidating config..."));
    EEPROM.begin(sizeof(EEConfig));
    eraseConfig();

    dlog.info(FPSTR(TAG), F("erase WiFi config..."));
    ESP.eraseConfig();

    dlog.info(FPSTR(TAG), F("waiting for power off!!!!"));
    dlog.end();

    //
    // delay one second then restart!
    //
    delay(1000);

    ESP.restart();
    while(true)
    {
    }
}

//
// prefix log lines with current time
//
void dlogPrefix(DLogBuffer& buffer, DLogLevel level)
{
    (void)level; // not used
    struct timeval tv;
    struct tm tm;
    gettimeofday(&tv, nullptr);
    gmtime_r(&tv.tv_sec, &tm);

    buffer.printf(F("%04d/%02d/%02d %02d:%02d:%02d.%06ld "),
            tm.tm_year+1900,
            tm.tm_mon+1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            tv.tv_usec);
}

void setup()
{
    static PROGMEM const char TAG[] = "setup";

    Serial.begin(115200);
    dlog.begin(new DLogPrintWriter(Serial));
    dlog.setPreFunc(&dlogPrefix);
    dlog.info(FPSTR(TAG), F("Startup!"));

    pinMode(SYNC_PIN, INPUT);
    pinMode(CONFIG_PIN, INPUT);

    feedback.off();

    //
    // lets read deep sleep data and see if we need to immed go back to sleep.
    //
    memset(&dsd, 0, sizeof(dsd));
    readDeepSleepData();

    Wire.begin();
    Wire.setClockStretchLimit(CLOCK_STRETCH_LIMIT);

    while (rtc.begin())
    {
        dlog.error(FPSTR(TAG), F("RTC begin failed! Attempting recovery..."));

        while (WireUtils.clearBus())
        {
            delay(10000);
            dlog.info(FPSTR(TAG), F("lets try that again..."));
        }
        delay(1000);
    }

    DS3231DateTime dt;
    getEdgeSyncedTime(dt, 1);
    struct timeval tv;
    tv.tv_sec  = dt.getUnixTime();
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);

    //
    // initialize config to defaults then load.
    memset(&config, 0, sizeof(config));
    config.sleep_duration = DEFAULT_SLEEP_DURATION;
    config.tz_offset = 0;

    //
    // make sure the device is available!
    //
    feedback.blink(0.9);

    dlog.info(FPSTR(TAG), F("starting clock interface"));
    while (clk.begin(3) != 0)
    {
        dlog.error(FPSTR(TAG), F("can't talk with Clock Controller!"));
        delay(10000);
    }

    uint8_t version = 0;
    while (clk.readVersion(&version, 3) != 0)
    {
        dlog.error(FPSTR(TAG), F("can't read clock status!"));
        delay(10000);
    }
    dlog.info(FPSTR(TAG), F("I2CAnalogClock VERSION: %u"), version);

    uint8_t clk_reason; // restart reason of clk
    while (clk.readResetReason(&clk_reason, 3) != 0)
    {
        dlog.error(FPSTR(TAG), F("can't read clock reset reason!"));
        delay(10000);
    }
    dlog.info(FPSTR(TAG), F("I2CAnalogClock restart reason: 0x%02x"), clk_reason);

    uint16_t pos;
    while (clk.readPosition(&pos, 3))
    {
        dlog.error(FPSTR(TAG), F("can't read clock position!"));
        delay(10000);
    }
    int hours = pos / 3600;
    int minutes = (pos - (hours * 3600)) / 60;
    int seconds = pos - (hours * 3600) - (minutes * 60);
    dlog.info(FPSTR(TAG), F("clock position: %d (%02d:%02d:%02d)"), pos, hours, minutes, seconds);


    bool clock_was_enabled = clk.getEnable();
    dlog.info(FPSTR(TAG), F("clock interface started, enabled:%s"), clock_was_enabled ? "true" : "false");

    // if the reset/config button is pressed then force config
    if (digitalRead(CONFIG_PIN) == 0)
    {
        //
        // If we wake with the reset button pressed and sleep_delay_left then the radio is off
        // so set it to 0 and use a very short deepSleep to turn the radio back on.
        //
        if (dsd.sleep_delay_left != 0)
        {
            dlog.info(FPSTR(TAG), F("reset button pressed with radio off, short sleep to enable!"));
            dlog.end();
            dsd.sleep_delay_left = 0;
            writeDeepSleepData();
            ESP.deepSleep(1, RF_DEFAULT); // super short sleep to enable the radio!
        }

        feedback.on();

        dlog.info(FPSTR(TAG), F("reset button pressed, forcing config or factory reset!"));
        force_config = true;
        //
        //  If we force config because of the config button then we stop the clock.
        //
        clk.writeAdjustment(0);
        clk.setEnable(false);

        //
        // if the buttin is held for FACTORY_RESET_DELAY milliseconds then factory reset!
        //
        time_t start = millis();
        while (digitalRead(CONFIG_PIN) == 0)
        {
            time_t now = millis();
            if ((now-start) > FACTORY_RESET_DELAY)
            {
                factoryReset();
            }
            delay(100);
        }
    }

    bool enabled = clk.getEnable();
    dlog.info(FPSTR(TAG), F("clock enable is:%u"), enabled);

    strncpy_P(config.ntp_server, PSTR(DEFAULT_NTP_SERVER), sizeof(config.ntp_server) - 1);
    config.ntp_server[sizeof(config.ntp_server) - 1] = 0;

    // Default to disabled (all tz_offsets = 0)
    config.tc[0].occurrence  = DEFAULT_TC0_OCCUR;
    config.tc[0].day_of_week = DEFAULT_TC0_DOW;
    config.tc[0].day_offset  = DEFAULT_TC0_DOFF;
    config.tc[0].month       = DEFAULT_TC0_MONTH;
    config.tc[0].hour        = DEFAULT_TC0_HOUR;
    config.tc[0].tz_offset   = DEFAULT_TC0_OFFSET;
    config.tc[1].occurrence  = DEFAULT_TC1_OCCUR;
    config.tc[1].day_of_week = DEFAULT_TC1_DOW;
    config.tc[1].day_offset  = DEFAULT_TC1_DOFF;
    config.tc[1].month       = DEFAULT_TC1_MONTH;
    config.tc[1].hour        = DEFAULT_TC1_HOUR;
    config.tc[1].tz_offset   = DEFAULT_TC1_OFFSET;

    dlog.debug(FPSTR(TAG), F("defaults: tz:%d ntp:%s logging: %s:%d"), config.tz_offset,
            config.ntp_server, config.network_logger_host, config.network_logger_port);

    dlog.debug(FPSTR(TAG), F("EEConfig size: %u"), sizeof(EEConfig));

    //
    // if the saved config was not good and the clock is not running
    // then force config.  If the clock is running then we trust that
    // it has the correct position.
    //
    if (!loadConfig() && !enabled)
    {
        force_config = true;
    }

#if defined(DEFAULT_LOG_ADDR) && defined(DEFAULT_LOG_PORT)
    if (strnlen(config.network_logger_host, 64) == 0)
    {
        strncpy(config.network_logger_host, DEFAULT_LOG_ADDR, 64);
        config.network_logger_port = DEFAULT_LOG_PORT;
    }
#endif

    dlog.info(FPSTR(TAG), F("config: tz:%d ntp:%s logging: %s:%d"), config.tz_offset,
            config.ntp_server, config.network_logger_host, config.network_logger_port);

    bool clock_needs_sync = updateTZOffset();

#if defined(USE_DRIFT)
    //
    // apply drift to RTC
    //
    if (setRTCfromDrift() == 0)
    {
        clock_needs_sync = true;
    }
#endif

    dlog.info(FPSTR(TAG), F("sleep_delay_left: %lu"), dsd.sleep_delay_left);

    if (dsd.sleep_delay_left != 0)
    {
        if (clock_needs_sync)
        {
            setCLKfromRTC();
        }
        sleepFor(dsd.sleep_delay_left);
    }

#if !defined(DISABLE_DEEP_SLEEP)
    if (!enabled)
    {
        force_config = true; // force the config portal to set the position if the clock is not running
    }
#endif

    initWiFi();
    //
    // Network started, log versions
    //
    dlog.info(FPSTR(TAG), F("SynchroClock VERSION: '%s'"), SYNCHRO_CLOCK_VERSION);
    dlog.info(FPSTR(TAG), F("I2CAnalogClock VERSION: %u"), version);

    dlog.info(FPSTR(TAG), F("process any OTA update"));
    processOTA(clock_was_enabled);

    dlog.debug(FPSTR(TAG), F("###### rtc data size: %d"), sizeof(RTCDeepSleepData));

    ntp.begin(NTP_PORT);

    if (!enabled)
    {
        dlog.info(FPSTR(TAG), F("enabling clock"));
        clk.setEnable(true);
    }
    else
    {
        dlog.info(FPSTR(TAG), F("clock is enabled, skipping init of RTC"));
    }

#if !defined(DISABLE_INITIAL_NTP)
    dlog.info(FPSTR(TAG), F("syncing RTC from NTP!"));
    setRTCfromNTP(config.ntp_server, true, NULL, NULL);
#endif

#if !defined(DISABLE_INITIAL_SYNC)
    dlog.info(FPSTR(TAG), F("syncing clock to RTC!"));
    setCLKfromRTC();
#endif

#if defined(DISABLE_DEEP_SLEEP)
    stay_awake = true;
#endif

    if (!stay_awake)
    {
#if defined(USE_NTP_POLL_ESTIMATE)
        sleepFor(ntp.getPollInterval());
#else
        sleepFor(config.sleep_duration);
#endif
    }

    dlog.info(FPSTR(TAG), F("starting HTTP"));
    HTTP.on("/offset",      HTTP_GET, handleOffset);
    HTTP.on("/adjust",      HTTP_GET, handleAdjustment);
    HTTP.on("/position",    HTTP_GET, handlePosition);
    HTTP.on("/tp_duration", HTTP_GET, handleTPDuration);
    HTTP.on("/tp_duty",     HTTP_GET, handleTPDuty);
    HTTP.on("/ap_duration", HTTP_GET, handleAPDuration);
    HTTP.on("/ap_duty",     HTTP_GET, handleAPDuty);
    HTTP.on("/ap_delay",    HTTP_GET, handleAPDelay);
    HTTP.on("/enable",      HTTP_GET, handleEnable);
    HTTP.on("/rtc",         HTTP_GET, handleRTC);
    HTTP.on("/ntp",         HTTP_GET, handleNTP);
    HTTP.on("/wire",        HTTP_GET, handleWire);
    HTTP.on("/save",        HTTP_GET, handleSave);
    HTTP.on("/erase",       HTTP_GET, handleErase);
    HTTP.on("/ap_start",    HTTP_GET, handleAPStartDuration);
    HTTP.on("/pwm_top",     HTTP_GET, handlePWMTop);
    HTTP.begin();
}

void sleepFor(uint32_t sleep_duration)
{
    static PROGMEM const char TAG[] = "sleepFor";

    dlog.info(FPSTR(TAG), F("seconds: %u"), sleep_duration);
    dsd.sleep_delay_left = sleep_duration;
    sleep_duration = MAX_SLEEP_DURATION;
    RFMode mode = RF_DEFAULT;
    if (dsd.sleep_delay_left > MAX_SLEEP_DURATION)
    {
        dsd.sleep_delay_left = dsd.sleep_delay_left - MAX_SLEEP_DURATION;
        mode = RF_DISABLED;
        dlog.info(FPSTR(TAG), F("sleep_duration > max, mode=DISABLED sleep_delay_left=%lu"), dsd.sleep_delay_left);
    }
    else
    {
        sleep_duration = dsd.sleep_delay_left;
        dsd.sleep_delay_left = 0;
        dlog.info(FPSTR(TAG), F("delay less than max, mode=DEFAULT sleep_delay_left=%lu"), dsd.sleep_delay_left);
    }

    writeDeepSleepData();
    dlog.info(FPSTR(TAG), F("Deep Sleep Time: %lu"), sleep_duration);
    dlog.end();
    ESP.deepSleep(sleep_duration * 1000000, mode);
}

void loop()
{
    if (stay_awake)
    {
        HTTP.handleClient();
    }
    delay(100);
}

int getEdgeSyncedTime(DS3231DateTime& dt, unsigned int retries)
{
    static PROGMEM const char TAG[] = "getEdgeSyncedTime";
    clk.waitForEdge(CLOCK_EDGE_FALLING);

    while(retries-- > 0)
    {
        clk.waitForEdge(CLOCK_EDGE_FALLING);
        // need 1ms delay!  but why?  noise?
        delay(1);
        if (rtc.readTime(dt) == 0)
        {
            return 0;
        }

        dlog.warning(FPSTR(TAG), F("failed to read from RTC, %d retries left"), retries);
        WireUtils.clearBus();
    }
    return -1;
}

int setRTCfromOffset(double offset, bool sync)
{
    static PROGMEM const char TAG[] = "setRTCfromOffset";

    int32_t  seconds = (int32_t)offset;
    uint32_t msdelay = fabs((offset - (double)seconds) * 1000);

    if (offset > 0)
    {
        seconds = seconds + 1; // +1 because we go to the next second
        msdelay = 1000.0 - msdelay;
    }

    dlog.info(FPSTR(TAG), F("offset: %lf seconds: %d msdelay: %d sync: %s"),
            offset, seconds, msdelay, sync ? "true" : "false");

    DS3231DateTime dt;

    if (getEdgeSyncedTime(dt, 3))
    {
        dlog.error(FPSTR(TAG), F("failed to read from RTC!"));
        return ERROR_RTC;
    }

    // wait for where the next second should start
    if (msdelay > 0 && msdelay < 1000)
    {
        delay(msdelay);
    }

    uint32_t old_time = dt.getUnixTime();
    uint32_t new_time = old_time + seconds;
    dt.setUnixTime(new_time);

    if (sync)
    {
        if (rtc.writeTime(dt))
        {
            dlog.error(FPSTR(TAG), F("FAILED to set RTC: %s"), dt.string());
            return ERROR_RTC;
        }
    }

    //
    // Update the TZ offset in case the timezone offset has changed based on new time.
    //
    delay(1); // DS3231 seems to need a short delay after write before we read the time back
    updateTZOffset();

    dlog.info(FPSTR(TAG), F("old_time: %d new_time: %d"), old_time, new_time);
    dlog.info(FPSTR(TAG), F("RTC: %s"), dt.string());
    return 0;
}

/*
 * sync to a second boundary and return the current time in seconds
 */
int getTime(uint32_t *result)
{
    clk.waitForEdge(CLOCK_EDGE_FALLING);
    delay(2); // ATtiny85 at 1mhz has interrupts disabled for a bit after the falling edge
    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dlog.error(F("getTime"), F("failed to read from RTC!"));
        return -1;
    }
    *result = dt.getUnixTime();
    return 0;
}

int setRTCfromDrift()
{
    static PROGMEM const char TAG[] = "setRTCfromDrift";
    double offset;
    if (ntp.getOffsetUsingDrift(&offset, &getTime))
    {
        dlog.error(FPSTR(TAG), F("failed, not adjusting for drift!"));
        return -1;
    }

    dlog.info(FPSTR(TAG), F("********* DRIFT OFFSET: %lf"), offset);

    int error = setRTCfromOffset(offset, true);
    if (error)
    {
        return error;
    }

    dlog.debug(FPSTR(TAG), F("returning OK"));
    return 0;
}

int setRTCfromNTP(const char* server, bool sync, double* result_offset, IPAddress* result_address)
{
    static PROGMEM const char TAG[] = "setRTCfromNTP";

    dlog.info(FPSTR(TAG), F("using server: %s"), server);

    double offset;
    if (ntp.getOffset(server, &offset, &getTime))
    {
        dlog.warning(FPSTR(TAG), F("NTP Failed!"));
        return ERROR_NTP;
    }

    if (result_address)
    {
        *result_address = ntp.getAddress();
    }

    if (result_offset != NULL)
    {
        *result_offset = offset;
    }

    dlog.info(FPSTR(TAG), F("********* NTP OFFSET: %lf"), offset);

    int error = setRTCfromOffset(offset, sync);
    if (error)
    {
        return error;
    }

    dlog.debug(FPSTR(TAG), F("returning OK"));
    return 0;
}

int setCLKfromRTC()
{
    static PROGMEM const char TAG[] = "setCLKfromRTC";

    // if there is already an adjustment in progress then stop it.
    if (clk.writeAdjustment(0))
    {
        dlog.error(FPSTR(TAG), F("failed to clear adjustment!"));
        return -1;
    }

#if defined(USE_STOP_THE_CLOCK)
    int delta;
    uint16_t clock_pos;
    uint16_t rtc_pos;
    do
    {
        clk.waitForEdge(CLOCK_EDGE_FALLING);
        delay(10);

        if (clk.readPosition(&clock_pos))
        {
            dlog.error(FPSTR(TAG), F("failed to read position, ignoring"));
            return -1;
        }
        dlog.info(FPSTR(TAG), F("clock position:%d"), clock_pos);

        DS3231DateTime dt;
        if (rtc.readTime(dt))
        {
            dlog.error(FPSTR(TAG), F("FAILED to read RTC"));
            return -1;
        }
        rtc_pos = dt.getPosition(config.tz_offset);
        dlog.info(FPSTR(TAG), F("RTC position:%d"), rtc_pos);

        delta = rtc_pos - clock_pos;
        if (delta < 0 && abs(delta) < STOP_THE_CLOCK_MAX)
        {
            int stop_for = abs(delta) + STOP_THE_CLOCK_EXTRA;
            dlog.info(FPSTR(TAG), F("stop the clock delta is %d stopping for %d seconds"), delta, stop_for);

            // stop the clock for delta seconds
            clk.setEnable(false);
            delay(stop_for * 1000);
            clk.setEnable(true);
        }
        //
        // if we stopped the clock then repeat block to re-read clock & RTC
        //
    } while (delta < 0 && abs(delta) < STOP_THE_CLOCK_MAX);
#else
    clk.waitForEdge(CLOCK_EDGE_FALLING);
    delay(10);
    uint16_t clock_pos;
    if (clk.readPosition(&clock_pos))
    {
        dlog.error(FPSTR(TAG), F("failed to read position, ignoring"));
        return -1;
    }
    dlog.info(FPSTR(TAG), F("clock position:%d"), clock_pos);

    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dlog.error(FPSTR(TAG), F("FAILED to read RTC"));
        return -1;
    }
    uint16_t rtc_pos = dt.getPosition(config.tz_offset);
    dlog.info(FPSTR(TAG), F("RTC position:%d"), rtc_pos);
#endif

    if (clock_pos != rtc_pos)
    {
        int adj = rtc_pos - clock_pos;
        if (adj < 0)
        {
            adj += MAX_POSITION;
        }
        dlog.info(FPSTR(TAG), F("sending adjustment of %u"), adj);
        clk.waitForEdge(CLOCK_EDGE_RISING);
        if (clk.writeAdjustment(adj))
        {
            dlog.error(FPSTR(TAG), F("failed to set adjustment!"));
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

void initConfig()
{
    if (EEPROM.length() != sizeof(EEConfig))
    {
        dlog.info(F("initConfig"), F("initializing EEPROM"));
        EEPROM.begin(sizeof(EEConfig));
    }
}

boolean loadConfig()
{
    static PROGMEM const char TAG[] = "loadConfig";
    EEConfig cfg;
    initConfig();
    // Read struct from EEPROM
    dlog.debug(FPSTR(TAG), F("loading from EEPROM"));
    unsigned int i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        p[i] = EEPROM.read(i);
    }
    dlog.debug(FPSTR(TAG), F("checking CRC"));
    uint32_t crcOfData = calculateCRC32(((uint8_t*) &cfg.data), sizeof(cfg.data));
    dlog.trace(FPSTR(TAG), F("CRC32 of data: %08x"), crcOfData);
    dlog.trace(FPSTR(TAG), F("CRC32 read from EEPROM: %08x"), cfg.crc);
    if (crcOfData != cfg.crc)
    {
        dlog.warning(FPSTR(TAG), F("CRC32 in EEPROM memory doesn't match CRC32 of data. Data is probably invalid!"));
        return false;
    }
    dlog.debug(FPSTR(TAG), F("CRC32 check ok, data is probably valid."));
    memcpy(&config, &cfg.data, sizeof(config));
    return true;
}

void saveConfig()
{
    static PROGMEM const char TAG[] = "saveConfig";
    EEConfig cfg;
    initConfig();
    memcpy(&cfg.data, &config, sizeof(cfg.data));
    cfg.crc = calculateCRC32(((uint8_t*) &cfg.data), sizeof(cfg.data));
    dlog.debug(FPSTR(TAG), F("caculated CRC: %08x"), cfg.crc);
    dlog.info(FPSTR(TAG), F("Saving configuration to EEPROM!"));

    unsigned int i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        EEPROM.write(i, p[i]);
    }
    bool result = EEPROM.commit();
    dlog.info(FPSTR(TAG), F("result: %s"), result ? "success" : "FAILURE");
}

void eraseConfig()
{
    static PROGMEM const char TAG[] = "eraseConfig";
    initConfig();
    dlog.info(FPSTR(TAG), F("erasing...."));
    for (unsigned int i = 0; i < sizeof(EEConfig); ++i)
    {
        EEPROM.write(i, 0xff);
    }
    dlog.info(FPSTR(TAG), F("committing...."));
    bool result = EEPROM.commit();
    dlog.info(FPSTR(TAG), F("result: %s"), result ? "success" : "FAILURE");
}

boolean readDeepSleepData()
{
    static PROGMEM const char TAG[] = "readDeepSleepData";
    RTCDeepSleepData rtcdsd;
    dlog.info(FPSTR(TAG), F("loading deep sleep data from RTC Memory"));
    if (!ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcdsd, sizeof(rtcdsd)))
    {
        dlog.error(FPSTR(TAG), F("failed to read RTC Memory"));
        return false;
    }

    uint32_t crcOfData = calculateCRC32(((uint8_t*) &rtcdsd.data), sizeof(rtcdsd.data));
    if (crcOfData != rtcdsd.crc)
    {
        dlog.warning(FPSTR(TAG), F("CRC32 in RTC Memory doesn't match CRC32 of data. Data is probably invalid!"));
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

    if (!ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcdsd, sizeof(rtcdsd)))
    {
        dlog.error(F("writeDeepSleepData"), F("failed to write RTC Memory"));
        return false;
    }
    return true;
}
