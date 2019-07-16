/*
 * ConfigParam.h
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

#ifndef CONFIGPARAM_H_
#define CONFIGPARAM_H_
#include <WiFiManager.h>
#include "Logger.h"

#include <functional>
typedef std::function<void(const char* result)> ConfigParamApply;

#define MAX_VALUE_LENGTH 63

class ConfigParam
{
public:
    ConfigParam(WiFiManager &wifi, const char *label);
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, int         value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint32_t    value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint16_t    value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint8_t     value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, double      value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, const char* value, int length, void (*applyCB)(const char* result));
    ~ConfigParam();
    void init(WiFiManager &wifi, const char *id, const char *placeholder, int length);
    boolean     isChanged();
    const char* getValue();
    const char* getId();
    void        apply();
    void        applyIfChanged();

private:
    WiFiManagerParameter* _wmp;
    ConfigParamApply      _apply;
    char                  _value[MAX_VALUE_LENGTH+1];
};

#endif /* CONFIGPARAM_H_ */
