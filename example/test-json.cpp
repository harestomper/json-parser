/*
 * test-json.cpp
 *
 *  Created on: 4 янв. 2016 г.
 *      Author: Voldemar Khramtsov <harestomper@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <cassert>
#include <iostream>

#include "json/json.h"

int main(int argc, char** argv)
{
    json::Value map = json::Map();

    map["bool"] = true;
    map["int"] = 1234;
    map["double"] = 1234.4321;
    map["string"] = "qwerty";
    map["array"] = { 123, 123.321, "3", 4U };
    map["array2"] = { };
    map["map1"] = json::Map({ { "key1", 123 }, { "key2", 123.321 }, { "key3", "asdfgghh" } });
    map["map2"] = json::Map();

    auto it = map.begin();
    auto end = map.end();
    while (it < end)
    {
        if (it->isUsed())
        {
            char const* key = map.getKey(it);
            printf("'%s'\n", key);
            printf("  isBool: %i\n", it->isBoolean());
            printf("  isInt: %i\n", it->isInteger());
            printf("  isDouble: %i\n", it->isDouble());
            printf("  isString: %i\n", it->isString());
            printf("  isArray: %i\n", it->isArray());
            printf("  isMap: %i\n", it->isMap());
            std::string val = *it;
            printf("  value: %s\n", val.c_str());
            printf("\n");
        }

        ++it;
    }

    int length = 0;
    char* str = map.saveToData(length, true);
    printf("%s\n", str);
    if (str)
        ::free(str);

    map.saveToFile("data.json", true);

    return 0;
}
