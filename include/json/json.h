/*
 * json.h
 *
 *  Created on: 18 янв. 2016 г.
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

#ifndef _JSON_JSON_H_
#define _JSON_JSON_H_

#ifndef null
#  if __cplusplus >= 201103L
#    define null nullptr
#  else
#    define null 0
#  endif
#endif /* null */

namespace json {

struct Value;
class Map;
class Array;
using Uint = unsigned int;
using Integer = long long int;
using iterator = Value*;
using const_iterator = Value const*;

struct MapBase
{
protected:
    Value*          m_values = null;
    Uint*           m_codes = null;
    char**          m_keys = null;
    Uint            m_num_buckets = 0;
    Uint            m_num_items = 0;
    Uint            m_capacity = 0;
};

struct ArrayBase
{
protected:
    Value*  m_data = null;
    Uint    m_num_items = 0;
    Uint    m_allocated_size = 0;
};

} /* namespace json */

#define BUFSIZE 1024

#define check_and_return(expr, ...) { \
    if (not (expr)) \
    { \
        char __b[BUFSIZE]; \
        ::snprintf(__b, BUFSIZE, " " __VA_ARGS__); \
        fprintf(::stderr, "ERROR:%s:%s:%i:'%s', failure. %s\n", __FILE__, __func__, __LINE__, #expr, __b); \
        return; \
    } }

#define check_and_return_val(expr, retval, ...) { \
    if (not (expr)) \
    { \
        char __b[BUFSIZE]; \
        ::snprintf(__b, BUFSIZE, " " __VA_ARGS__); \
        fprintf(::stderr, "ERROR:%s:%s:%i:'%s', failure. %s\n", __FILE__, __func__, __LINE__, #expr, __b); \
        return (retval); \
    } }

#include "jsonvalue.h"
#include "jsonarray.h"
#include "jsonmap.h"

#endif /* _JSON_JSON_H_ */
