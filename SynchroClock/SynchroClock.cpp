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

#define DEBUG
#include "Logger.h"

//
// These are only used when debugging
//
//#define DISABLE_DEEP_SLEEP
//#define DISABLE_INITIAL_NTP
//#define DISABLE_INITIAL_SYNC
//#define USE_BUILTIN_LED
//#define HARD_CODED_WIFI

//#define DEFAULT_LOG_ADDR "192.168.0.42"
//#define DEFAULT_LOG_PORT 1421

#if defined(USE_BUILTIN_LED)
#undef LED_PIN
#define LED_PIN BUILTIN_LED
#endif

#ifdef HARD_CODED_WIFI
#define HARD_CODED_SSID "Whatever"
#define HARD_CODED_PASS "the password"
// define to declare clock position if its not already running.
//#define HARD_CODED_POSITION "00:00:00"
#endif

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

char message[128]; // buffer for http return values

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
        dbprintf("parseDuty: invalid value %s: using 50 instead!\n", value);
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
        dbprintf("getValidByte: invalid value %d: using 255 instead!\n", i);
        i = 255;
    }
    return (uint8_t) i;
}

void handleOffset()
{
    if (HTTP.hasArg("set"))
    {
        config.tz_offset = getValidOffset("set");
        dbprintf("seconds offset:%d\n", config.tz_offset);
    }

    HTTP.send(200, "text/plain", String(config.tz_offset) + "\n");
}

void handleAdjustment()
{
    uint16_t adj;
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

    HTTP.send(200, "text/plain", message);
}

void handlePosition()
{
    uint16_t pos;
    if (HTTP.hasArg("set"))
    {
        pos = getValidPosition("set");
        dbprintf("setting position:%u\n", pos);
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
        sprintf(message, "position: %d (%02d:%02d:%02d)\n", pos, hours, minutes, seconds);
    }

    HTTP.send(200, "text/Plain", message);
}

void handleTPDuration()
{
    uint8_t value;
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

    HTTP.send(200, "text/plain", message);
}

void handleTPDuty()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuty("set");
        dbprintf("setting tp_duty:%u\n", value);
        if (clk.writeTPDuty(value))
        {
            dbprintln("failed to set TP duty!");
        }
    }

    if (clk.readTPDuty(&value))
    {
        sprintf(message, "failed to read TP duty\n");
    }
    else
    {
        sprintf(message, "TP duty: %u\n", value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPDuration()
{
    uint8_t value;
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

    HTTP.send(200, "text/plain", message);
}

void handleAPDuty()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuty("set");
        dbprintf("setting ap_duty:%u\n", value);
        if (clk.writeAPDuty(value))
        {
            dbprintln("failed to set AP duty!");
        }
    }

    if (clk.readAPDuty(&value))
    {
        sprintf(message, "failed to read AP duty\n");
    }
    else
    {
        sprintf(message, "AP duty: %u\n", value);
    }

    HTTP.send(200, "text/plain", message);
}

void handleAPDelay()
{
    uint8_t value;
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

    HTTP.send(200, "text/plain", message);
}

void handleAPStartDuration()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidDuration("set");
        dbprintf("setting ap_start:%u\n", value);
        if (clk.writeAPStartDuration(value))
        {
            dbprintln("failed to set AP start duration");
        }
    }

    if (clk.readAPStartDuration(&value))
    {
        sprintf(message, "failed to read AP start duration\n");
    }
    else
    {
        sprintf(message, "AP start duration: %u\n", value);
    }

    HTTP.send(200, "text/plain", message);
}

void handlePWMTop()
{
    uint8_t value;
    if (HTTP.hasArg("set"))
    {
        value = getValidByte("set");
        dbprintf("setting pwm_top:%u\n", value);
        if (clk.writePWMTop(value))
        {
            dbprintln("failed to set pwm_top!");
        }
    }

    if (clk.readPWMTop(&value))
    {
        sprintf(message, "failed to read pwm_top!\n");
    }
    else
    {
        sprintf(message, "pwm_top: %u\n", value);
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
    char message[64];

    DS3231DateTime dt;
    int err = rtc.readTime(dt);

    if (!err)
    {
        dbprintf("handleRTC: RTC : %s (UTC)\n", dt.string());

        if (HTTP.hasArg("offset"))
        {
            double offset = strtod(HTTP.arg("offset").c_str(), NULL);
            dbprintf("handleRTC: offset: %lf\n", offset);
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
        sprintf(message, "%d (%02d:%02d:%02d)\n", value, hours, minutes, seconds);
    }
    else
    {
        sprintf(message, "rtc.readTime() failed!\n");
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
        sync = getValidBoolean("sync");
    }

    double offset;
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

        snprintf(message, 64, "OFFSET: %0.6lf (%s)\n", offset, address.toString().c_str());
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

void handleSave()
{
    clk.saveConfig();
    saveConfig();
    HTTP.send(200, "text/plain", "Saved!\n");
}

void handleErase()
{
    for (unsigned int i = 0; i < sizeof(EEConfig); ++i)
    {
        EEPROM.write(i, 0);
    }
    EEPROM.commit();
    HTTP.send(200, "text/plain", "Erased!\n");
}


//
// update the timezone offset based on the current date/time
// return true if offset was updated
//
bool updateTZOffset()
{
    DS3231DateTime dt;
    if (rtc.readTime(dt))
    {
        dbprintln("updateTZOffset: FAILED to read RTC");
        return false;
    }

    int new_offset = TimeUtils::computeUTCOffset(dt.getUnixTime(), config.tz_offset, config.tc, TIME_CHANGE_COUNT);

    // if the time zone changed then save the new value and return true
    if (config.tz_offset != new_offset)
    {
        dbprintf("time zone offset changed from %d to %d\n", config.tz_offset, new_offset);
        config.tz_offset = new_offset;
        saveConfig();
        return true;
    }

    return false;
}

void initWiFi()
{
    dbprintln("starting wifi!");

#if defined(LED_PIN)
    // setup wifi, blink let slow while connecting and fast if portal activated.
    feedback.blink(FEEDBACK_LED_SLOW);
#endif

    WiFiManager wifi;
    wifi.setDebugOutput(false);
    wifi.setConnectTimeout(CONNECTION_TIMEOUT);

    wifi.setSaveConfigCallback([]()
    {
        save_config = true;
    });
#if defined(LED_PIN)
    wifi.setAPCallback([](WiFiManager *)
    {
        feedback.blink(FEEDBACK_LED_FAST);
    });
#endif
    uint16_t pos;
    char pos_str[16];
    clk.readPosition(&pos);
    int hours = pos / 3600;
    int minutes = (pos - (hours * 3600)) / 60;
    int seconds = pos - (hours * 3600) - (minutes * 60);
    memset(pos_str, 0, sizeof(pos_str));
    snprintf(pos_str, 15, "%02d:%02d:%02d", hours, minutes, seconds);

    ConfigParam position(wifi, "position", "Clock Position", pos_str, 10, [](const char* result)
    {
        if (strlen(result))
        {
            uint16_t position = TimeUtils::parsePosition(result);
            dbprintf("setting position to %d\n", position);
            if (clk.writePosition(position))
            {
                dbprintln("failed to set initial position!");
            }
        }
    });

    ConfigParam ntp_server(wifi, "ntp_server", "NTP Server", config.ntp_server, 32, [](const char* result)
    {
        strncpy(config.ntp_server, result, sizeof(config.ntp_server) - 1);
    });

    ConfigParam tc1_label(wifi, "<p>1st Time Change</p>");
    ConfigParam tc1_occurence(wifi, "tc1_occurrence", "occurrence", config.tc[0].occurrence, 3, [](const char* result)
    {
        config.tc[0].occurrence = TimeUtils::parseOccurrence(result);
    });
    ConfigParam tc1_day_of_week(wifi, "tc1_day_of_week", "Day of Week (Sun=0)", config.tc[0].day_of_week, 2, [](const char* result)
    {
        config.tc[0].day_of_week = TimeUtils::parseDayOfWeek(result);
    });
    ConfigParam tc1_day_offset(wifi, "tc1_day_offset", "day offset", config.tc[0].day_offset, 3, [](const char* result)
    {
        config.tc[0].day_offset = TimeUtils::parseOccurrence(result);
    });
    ConfigParam tc1_month(wifi, "tc1_month", "Month (Jan=1)", config.tc[0].month, 3, [](const char* result)
    {
        config.tc[0].month = TimeUtils::parseMonth(result);
    });
    ConfigParam tc1_hour(wifi, "tc1_hour", "Hour (0-23)", config.tc[0].hour, 3, [](const char* result)
    {
        config.tc[0].hour = TimeUtils::parseHour(result);
    });
    ConfigParam tc1_offset(wifi, "tc1_offset", "Time Offset", config.tc[0].tz_offset, 8, [](const char* result)
    {
        config.tc[0].tz_offset = TimeUtils::parseOffset(result);
    });

    ConfigParam tc2_label(wifi, "<p>2nd Time Change</p>");
    ConfigParam tc2_occurence(wifi, "tc2_occurrence", "occurrence", config.tc[1].occurrence, 3, [](const char* result)
    {
        config.tc[1].occurrence = TimeUtils::parseOccurrence(result);
    });
    ConfigParam tc2_day_of_week(wifi, "tc2_day_of_week", "Day of Week (Sun=0)", config.tc[1].day_of_week, 2, [](const char* result)
    {
        config.tc[1].day_of_week = TimeUtils::parseDayOfWeek(result);
    });
    ConfigParam tc2_day_offset(wifi, "tc2_day_offset", "day offset", config.tc[1].day_offset, 3, [](const char* result)
    {
        config.tc[1].day_offset = TimeUtils::parseOccurrence(result);
    });
    ConfigParam tc2_month(wifi, "tc2_month", "Month (Jan=1)", config.tc[1].month, 3, [](const char* result)
    {
        config.tc[1].month = TimeUtils::parseMonth(result);
    });
    ConfigParam tc2_hour(wifi, "tc2_hour", "Hour (0-23)", config.tc[1].hour, 3, [](const char* result)
    {
        config.tc[1].hour = TimeUtils::parseHour(result);
    });
    ConfigParam tc2_offset(wifi, "tc2_offset", "Time Offset", config.tc[1].tz_offset, 8, [](const char* result)
    {
        config.tc[1].tz_offset = TimeUtils::parseOffset(result);
    });

    ConfigParam advance_label(wifi, "<p>Advanced Settings!</p>");
    ConfigParam no_sleep(wifi, "stay_awake", "Stay Awake 'true'", "", 8, [](const char* result)
    {
        stay_awake = parseBoolean(result);
        dbprintf("no_sleep: result:'%s' -> stay_awake:%d\n", result, stay_awake);
    });
    ConfigParam sleep_duration(wifi, "sleep_duration", "Sleep", config.sleep_duration, 8, [](const char* result)
    {
        config.sleep_duration = atoi(result);
    });
    uint8_t value;
    clk.readTPDuration(&value);
    ConfigParam tp_duration(wifi, "tp_duration", "Tick Pulse", value, 8, [](const char* result)
    {
        uint8_t tp_duration = TimeUtils::parseSmallDuration(result);
        clk.writeTPDuration(tp_duration);
    });
    clk.readTPDuty(&value);
    ConfigParam tp_duty(wifi, "tp_duty", "Tick Pulse Duty", value, 8, [](const char* result)
    {
        uint8_t tp_duty = parseDuty(result);
        clk.writeTPDuty(tp_duty);
    });
    clk.readAPStartDuration(&value);
    ConfigParam ap_start(wifi, "ap_start", "Adjust Start Pulse", value, 4, [](const char* result)
    {
        uint8_t ap_start = TimeUtils::parseSmallDuration(result);
        clk.writeAPStartDuration(ap_start);
    });
    clk.readAPDuration(&value);
    ConfigParam ap_duration(wifi, "ap_duration", "Adjust Pulse", value, 4, [](const char* result)
    {
        uint8_t ap_duration = TimeUtils::parseSmallDuration(result);
        clk.writeAPDuration(ap_duration);
    });
    clk.readAPDuty(&value);
    ConfigParam ap_duty(wifi, "ap_duty", "Adjust Pulse Duty", value, 8, [](const char* result)
    {
        uint8_t ap_duty = parseDuty(result);
        clk.writeAPDuty(ap_duty);
    });
    clk.readAPDelay(&value);
    ConfigParam ap_delay(wifi, "ap_delay", "Adjust Delay", value, 4, [](const char* result)
    {
        uint8_t ap_delay = TimeUtils::parseSmallDuration(result);
        clk.writeAPDelay(ap_delay);
    });
    ConfigParam network_logger_host(wifi, "network_logger_host", "Network Log Host", config.network_logger_host, 32, [](const char* result)
    {
        strncpy(config.network_logger_host, result, sizeof(config.network_logger_host) - 1);
    });
    ConfigParam network_logger_port(wifi, "network_logger_port", "Network Log Port", config.network_logger_port, 6, [](const char* result)
    {
        config.network_logger_port = atoi(result);
    });
    ConfigParam clear_ntp_persist(wifi, "clear_ntp_persist", "Clear NTP Persist 'true'", "", 8, [](const char* result)
    {
        boolean clearit = parseBoolean(result);
        dbprintf("clear_ntp_persist: result:'%s' -> clear_ntp_persist:%d\n", result, clearit);
        if (clearit)
        {
            memset(&config.ntp_persist, 0, sizeof(config.ntp_persist));
        }
    });
    String ssid = "SynchroClock" + String(ESP.getChipId());
    dbflush();
#if defined(HARD_CODE_WIFI)
    config.network_logger_host[0] = 0;
    save_config = true;
#if defined(HARD_CODED_POSITION)
    if (!clk.getEnable())
    {
        uint16_t position = TimeUtils::parsePosition(HARD_CODED_POSITION);
        dbprintf("setting position to %d\n", position);
        if (clk.writePosition(position))
        {
            dbprintln("failed to set initial position!");
        }
    }
#endif
    Serial.print("HACK: Connecting to wifi");
    WiFi.begin(HARD_CODED_SSID, HARD_CODED_PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("done!");
#else
    if (force_config)
    {
        wifi.startConfigPortal(ssid.c_str(), NULL);
    }
    else
    {
        wifi.autoConnect(ssid.c_str(), NULL);
    }
#endif
#if defined(LED_PIN)
    feedback.off();
#endif
    //
    // if we are not connected then deep sleep and try again.
    if (!WiFi.isConnected())
    {
        dbprintln("failed to connect to wifi!");
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
        position.applyIfChanged();
        ntp_server.applyIfChanged();
        tc1_occurence.applyIfChanged();
        tc1_day_of_week.applyIfChanged();
        tc1_day_offset.applyIfChanged();
        tc1_month.applyIfChanged();
        tc1_hour.applyIfChanged();
        tc1_offset.applyIfChanged();
        tc2_occurence.applyIfChanged();
        tc2_day_of_week.applyIfChanged();
        tc2_day_offset.applyIfChanged();
        tc2_month.applyIfChanged();
        tc2_hour.applyIfChanged();
        tc2_offset.applyIfChanged();
        no_sleep.applyIfChanged();
        tp_duration.applyIfChanged();
        tp_duty.applyIfChanged();
        ap_start.applyIfChanged();
        ap_duration.applyIfChanged();
        ap_duty.applyIfChanged();
        ap_delay.applyIfChanged();
        sleep_duration.applyIfChanged();
        network_logger_host.applyIfChanged();
        network_logger_port.applyIfChanged();
        clear_ntp_persist.applyIfChanged();
        clk.saveConfig();
        saveConfig();
        updateTZOffset();
    }

    // Configure network logging if enabled
    dbnetlog(config.network_logger_host, config.network_logger_port);

    IPAddress ip = WiFi.localIP();
    dbprintf("Connected! IP address is: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
}


void setup()
{
    dbbegin(115200);
    dbprintln("");
    dbprintln("Startup!");

    pinMode(SYNC_PIN, INPUT);
    pinMode(CONFIG_PIN, INPUT);
#if defined(LED_PIN)
    feedback.off();
#endif
    //
    // lets read deep sleep data and see if we need to immed go back to sleep.
    //
    memset(&dsd, 0, sizeof(dsd));
    readDeepSleepData();

    Wire.begin();
    Wire.setClockStretchLimit(CLOCK_STRETCH_LIMIT);

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

    //
    // initialize config to defaults then load.
    memset(&config, 0, sizeof(config));
    config.sleep_duration = DEFAULT_SLEEP_DURATION;
    config.tz_offset = 0;

    //
    // make sure the device is available!
    //
#if defined(LED_PIN)
    feedback.blink(0.9);
#endif
    dbprintln("starting clock interface");
    while (clk.begin())
    {
        dbprintln("can't talk with Clock Controller!");
        while (WireUtils.clearBus())
        {
            delay(10000);
            dbprintln("lets try that again...");
        }
        delay(10000);
    }
    dbprintln("clock interface started");

    // if the reset/config button is pressed then force config
    if (digitalRead(CONFIG_PIN) == 0)
    {
        //
        // If we wake with the reset button pressed and sleep_delay_left then the radio is off
        // so set it to 0 and use a very short deepSleep to turn the radio back on.
        //
        if (dsd.sleep_delay_left != 0)
        {
            dbprintln("reset button pressed with radio off, short sleep to enable!");
            dsd.sleep_delay_left = 0;
            writeDeepSleepData();
            ESP.deepSleep(1, RF_DEFAULT); // super short sleep to enable the radio!
        }

#if defined(LED_PIN)
        feedback.on();
#endif
        dbprintln("waiting for config release");
        while (digitalRead(CONFIG_PIN) == 0)
        {
            // wait for it to be let up.
            delay(10);
        }


        //
        // now a short delay, if the button is pressed again after that then we use stay
        // awake mode and start a web server.
        //
        dbprintln("short delay to see if its pressed again.");
        delay(2000);

#if defined(LED_PIN)
        feedback.off();
#endif

        if (digitalRead(CONFIG_PIN) == 0)
        {
            dbprintln("Its pressed, use stay awake!");
            stay_awake = true;
        }
        else
        {
            dbprintln("reset button pressed, forcing config!");
            force_config = true;
            //
            //  If we force config because of the config button then we stop the clock.
            //
            clk.setEnable(false);
        }
    }


    bool enabled = clk.getEnable();
    dbprintf("clock enable is:%u\n", enabled);

    strncpy(config.ntp_server, DEFAULT_NTP_SERVER, sizeof(config.ntp_server) - 1);
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

    dbprintf("defaults: tz:%d ntp:%s logging: %s:%d\n", config.tz_offset,
            config.ntp_server, config.network_logger_host, config.network_logger_port);

    dbprintf("EEConfig size: %u\n", sizeof(EEConfig));

    EEPROM.begin(sizeof(EEConfig));
    delay(100);

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

    dbprintf("config: tz:%d ntp:%s logging: %s:%d\n", config.tz_offset,
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

    dbprintf("sleep_delay_left: %lu\n", dsd.sleep_delay_left);

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

    dbprintf("###### rtc data size: %d\n", sizeof(RTCDeepSleepData));

    ntp.begin(NTP_PORT);

    if (!enabled)
    {
        dbprintln("enabling clock");
        clk.setEnable(true);
    }
    else
    {
        dbprintln("clock is enabled, skipping init of RTC");
    }

#if !defined(DISABLE_INITIAL_NTP)
    dbprintln("syncing RTC from NTP!");
    setRTCfromNTP(config.ntp_server, true, NULL, NULL);
#endif

#if !defined(DISABLE_INITIAL_SYNC)
    dbprintln("syncing clock to RTC!");
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

    dbprintln("starting HTTP");
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
    dbprintf("sleepFor: %u\n", sleep_duration);
    dsd.sleep_delay_left = sleep_duration;
    sleep_duration = MAX_SLEEP_DURATION;
    RFMode mode = RF_DEFAULT;
    if (dsd.sleep_delay_left > MAX_SLEEP_DURATION)
    {
        dsd.sleep_delay_left = dsd.sleep_delay_left - MAX_SLEEP_DURATION;
        mode = RF_DISABLED;
        dbprintf("sleepFor: sleep_duration > max, mode=DISABLED sleep_delay_left=%lu\n", dsd.sleep_delay_left);
    }
    else
    {
        sleep_duration = dsd.sleep_delay_left;
        dsd.sleep_delay_left = 0;
        dbprintf("sleepFor: delay less than max, mode=DEFAULT sleep_delay_left=%lu\n", dsd.sleep_delay_left);
    }

    writeDeepSleepData();
    dbprintf("Deep Sleep Time: %lu\n", sleep_duration);
    dbflush();
    dbend();
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

int setRTCfromOffset(double offset, bool sync)
{
    int32_t  seconds = (int32_t)offset;
    uint32_t msdelay = fabs((offset - (double)seconds) * 1000);

    if (offset > 0)
    {
        seconds = seconds + 1; // +1 because we go to the next second
        msdelay = 1000.0 - msdelay;
    }

    dbprintf("setRTCfromOffset:: offset: %lf seconds: %d msdelay: %d sync: %s\n",
            offset, seconds, msdelay, sync ? "true" : "false");

    DS3231DateTime dt;

    clk.waitForEdge(CLOCK_EDGE_FALLING);

    if (rtc.readTime(dt))
    {
        dbprintln("setRTCfromOffset: failed to read from RTC, attempting corrective action...");
        WireUtils.clearBus();
        if (rtc.readTime(dt))
        {
            dbprintln("setRTCfromOffset: failed to read from RTC!");
            return ERROR_RTC;
        }
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
            dbprintf("setRTCfromOffset: FAILED to set RTC: %s\n", dt.string());
            return ERROR_RTC;
        }
    }

    //
    // Update the TZ offset in case the timezone offset has changed based on new time.
    //
    delay(1); // DS3231 seems to need a short delay after write before we read the time back
    updateTZOffset();

    dbprintf("setRTCfromOffset: old_time: %d new_time: %d\n", old_time, new_time);
    dbprintf("setRTCfromOffset: RTC: %s\n", dt.string());
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
        dbprintln("getTime: failed to read from RTC!");
        return -1;
    }
    *result = dt.getUnixTime();
    return 0;
}

int setRTCfromDrift()
{
    double offset;
    if (ntp.getOffsetUsingDrift(&offset, &getTime))
    {
        dbprintln("setRTCfromDrift: failed, not adjusting for drift!");
        return -1;
    }

    dbprintf("********* DRIFT OFFSET: %lf\n", offset);

    int error = setRTCfromOffset(offset, true);
    if (error)
    {
        return error;
    }

    dbprintln("setRTCfromDrift: returning OK");
    return 0;
}

int setRTCfromNTP(const char* server, bool sync, double* result_offset, IPAddress* result_address)
{
    dbprintf("using server: %s\n", server);

    double offset;
    if (ntp.getOffset(server, &offset, &getTime))
    {
        dbprintf("setRTCfromNTP: NTP Failed!\n");
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

    dbprintf("********* NTP OFFSET: %lf\n", offset);

    int error = setRTCfromOffset(offset, sync);
    if (error)
    {
        return error;
    }

    dbprintln("setRTCfromNTP: returning OK");
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
        rtc_pos = dt.getPosition(config.tz_offset);
        dbprintf("setCLKfromRTC: RTC position:%d\n", rtc_pos);

        delta = rtc_pos - clock_pos;
        if (delta < 0 && abs(delta) < STOP_THE_CLOCK_MAX)
        {
            int stop_for = abs(delta) + STOP_THE_CLOCK_EXTRA;
            dbprintf("setCLKfromRTC: stop the clock delta is %d stopping for %d seconds\n", delta, stop_for);

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
#endif

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
    dbprintln("loadConfig: loading from EEPROM");
    unsigned int i;
    uint8_t* p = (uint8_t*) &cfg;
    for (i = 0; i < sizeof(cfg); ++i)
    {
        p[i] = EEPROM.read(i);
        //dbprintf("loadConfig: p[%u] = %u\n", i, p[i]);
    }
    dbprintln("loadConfig: checking CRC");
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
    dbprintln("Saving configuration to EEPROM!");

    unsigned int i;
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
    if (!ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcdsd, sizeof(rtcdsd)))
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

    if (!ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcdsd, sizeof(rtcdsd)))
    {
        dbprintln("writeDeepSleepData: failed to write RTC Memory");
        return false;
    }
    return true;
}
