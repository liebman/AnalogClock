/*
 * ConfigParam.cpp
 *
 *  Created on: Jun 6, 2017
 *      Author: chris.l
 */

#include "ConfigParam.h"

#ifdef DEBUG_CONFIG_PARAM
#define DBP_BUF_SIZE 256
#define dbprintf(...) logger.printf(__VA_ARGS__)
#define dbprintln(x)  logger.println(x)
#define dbflush()     logger.flush()
#else
#define dbprintf(...)
#define dbprintln(x)
#define dbflush()
#endif

ConfigParam::ConfigParam(WiFiManager &wifi, const char *label)
{
    _apply = NULL;
    _wmp = new WiFiManagerParameter(label);
    wifi.addParameter(_wmp);
}

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, const char* value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    strncpy(_value, value, MAX_VALUE_LENGTH);
    _value[MAX_VALUE_LENGTH] = 0;
    init(wifi, id, placeholder, length);
}

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, int value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    snprintf(_value, MAX_VALUE_LENGTH, "%d", value);
    init(wifi, id, placeholder, length);
}

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint32_t value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    snprintf(_value, MAX_VALUE_LENGTH, "%u", value);
    init(wifi, id, placeholder, length);
}

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint16_t value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    snprintf(_value, MAX_VALUE_LENGTH, "%u", value);
    init(wifi, id, placeholder, length);
}

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint8_t value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    snprintf(_value, MAX_VALUE_LENGTH, "%u", value);
    init(wifi, id, placeholder, length);
}

ConfigParam::~ConfigParam()
{
    if (_wmp != NULL)
    {
        free(_wmp);
    }
}

void ConfigParam::init(WiFiManager &wifi, const char *id, const char *placeholder, int length)
{
    dbprintf("ConfigParam::init: id:%s value:'%s'\n", id, _value);
    _wmp = new WiFiManagerParameter(id, placeholder, _value, length);
    wifi.addParameter(_wmp);
}

boolean ConfigParam::isChanged()
{
    if (strcmp(_wmp->getValue(), _value))
    {
        return true;
    }
    dbprintf("ConfigParam::isChanged: id:%s old:'%s', new:'%s'\n", _wmp->getID(), _value, _wmp->getValue());
    return false;
}

void ConfigParam::apply()
{
    if (_apply != NULL)
    {
        dbprintf("ConfigParam::apply: id:%s value:'%s'\n", _wmp->getID(), _wmp->getValue());
        _apply(_wmp->getValue());
    }
}

void ConfigParam::applyIfChanged()
{
    if (isChanged())
    {
        apply();
    }
}

const char* ConfigParam::getValue()
{
    return _wmp->getValue();
}

int ConfigParam::getIntValue()
{
    return atoi(_wmp->getValue());
}

