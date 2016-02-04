/*
 * jsonarray.h
 *
 *  Created on: 9 янв. 2016 г.
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

#ifndef _JSON_JSONARRAY_H_
#define _JSON_JSONARRAY_H_

#include <initializer_list>
#include "json.h"

namespace json {

class Array : protected ArrayBase
{
public:
    Array(Uint initial_size = 0);
    Array(json::const_iterator first, json::const_iterator last);
    Array(std::initializer_list<Value> const& list);
    Array(Array const& other);
    Array(Array&& other);
    ~Array();
    Array& operator=(Array const& other);
    Array& operator=(Array&& other);

    void assign(Array const& other);
    void assign(json::const_iterator first, json::const_iterator last);
    void swap(Array& other);
    void clear();

    Uint numItems() const;
    Uint reserved() const;
    Value* data() const;

    void reserve(Uint new_size);
    void resize(Uint new_size);

    void insert(Uint index, Value const& value);
    void insert(Uint index, json::const_iterator first, json::const_iterator last);

    void append(Value const& value);
    void append(json::const_iterator first, json::const_iterator last);

    void prepend(Value const& value);
    void prepend(json::const_iterator first, json::const_iterator last);

    void remove(Uint index);

    Value& operator[](Uint index);
    Value const& operator[](Uint index) const;

    json::iterator begin();
    json::const_iterator begin() const;
    json::iterator end();
    json::const_iterator end() const;
};

}  // namespace json


#endif /* _JSON_JSONARRAY_H_ */
