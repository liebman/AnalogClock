/*
 * ConfigParam.h
 *
 *  Created on: Jun 6, 2017
 *      Author: chris.l
 */

#ifndef CONFIGPARAM_H_
#define CONFIGPARAM_H_
#include <WiFiManager.h>
#include "Logger.h"

#define DEBUG_CONFIG_PARAM
#define MAX_VALUE_LENGTH 63

class ConfigParam
{
public:
    ConfigParam(WiFiManager &wifi, const char *label);
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, int         value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint32_t    value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint16_t    value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, uint8_t     value, int length, void (*applyCB)(const char* result));
    ConfigParam(WiFiManager &wifi, const char *id, const char *placeholder, const char* value, int length, void (*applyCB)(const char* result));
    ~ConfigParam();
    void init(WiFiManager &wifi, const char *id, const char *placeholder, int length);
    boolean     isChanged();
    const char* getValue();
    int         getIntValue();
    void        apply();
    void        applyIfChanged();

private:
    WiFiManagerParameter* _wmp;
    void                  (*_apply)(const char* result);
    char                  _value[MAX_VALUE_LENGTH+1];
};

#endif /* CONFIGPARAM_H_ */
