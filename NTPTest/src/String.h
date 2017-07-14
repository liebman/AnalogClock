/*
 * String.h
 *
 *  Created on: Jul 8, 2017
 *      Author: liebman
 */

#ifndef STRING_H_
#define STRING_H_

class String
{
public:
    String();
    String(const char* str);
    virtual ~String();
    const char* c_str();
private:
    char* value;
};

#endif /* STRING_H_ */
