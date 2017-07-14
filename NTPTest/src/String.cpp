/*
 * String.cpp
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#include "String.h"
#include <stdlib.h>
#include <string.h>

String::String()
{
    value = NULL;
}
String::String(const char* str)
{
    value = (char*)malloc(strlen(str)+1);
    strcpy(value, str);
}

String::~String()
{
    if (value != NULL)
    {
        free(value);
        value = NULL;
    }
}

const char* String::c_str()
{
    return value;
}
