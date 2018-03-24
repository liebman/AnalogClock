/*
 * ConfigParam.cpp
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
 *  Created on: Jun 6, 2017
 *      Author: chris.l
 */

#include "ConfigParam.h"

static PROGMEM const char TAG[] = "ConfigParam";

ConfigParam::ConfigParam(WiFiManager &wifi, const char *label)
{
    _apply = nullptr;
    _wmp = new WiFiManagerParameter(label);
    dlog.trace(FPSTR(TAG), F("ConfigParam: create label='%s'"), label);
    _value[0] = '\0';
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

ConfigParam::ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, double value, int length, void (*applyCB)(const char*))
{
    _apply = applyCB;
    snprintf(_value, MAX_VALUE_LENGTH, "%lf", value);
    init(wifi, id, placeholder, length);
}

ConfigParam::~ConfigParam()
{
    if (_wmp != NULL)
    {
        const char* id = "<unknown>";
        if (_wmp->getID() != nullptr)
        {
            id = _wmp->getID();
        }
        else if (_wmp->getCustomHTML() != nullptr)
        {
            id = _wmp->getCustomHTML();
        }
        dlog.trace(FPSTR(TAG), F("~ConfigParam: destroying '%s'"), id);
        delete _wmp;
        _wmp = nullptr;
    }
}

void ConfigParam::init(WiFiManager &wifi, const char *id, const char *placeholder, int length)
{
    dlog.debug(FPSTR(TAG), F("::init: id:%s value:'%s'"), id, _value);
    _wmp = new WiFiManagerParameter(id, placeholder, _value, length);
    wifi.addParameter(_wmp);
}

boolean ConfigParam::isChanged()
{
    dlog.trace(FPSTR(TAG), F("::isChanged: _wmp: 0x%08x"), (uint32_t)_wmp);

    if (_wmp != nullptr && _wmp->getValue() != nullptr)
    {
        dlog.trace(FPSTR(TAG), F("::isChanged: id:%s old:'%s', new:'%s'"), _wmp->getID(), _value, _wmp->getValue());
        if (strcmp(_wmp->getValue(), _value))
        {
            dlog.debug(FPSTR(TAG), F("::isChanged: true!"));
            return true;
        }
    }
    return false;
}

void ConfigParam::apply()
{
    if (_apply != nullptr)
    {
        dlog.debug(FPSTR(TAG), F("::apply: id:%s value:'%s'"), _wmp->getID(), _wmp->getValue());
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

const char* ConfigParam::getId()
{
    return _wmp->getID();
}

const char* ConfigParam::getValue()
{
    return _wmp->getValue();
}
