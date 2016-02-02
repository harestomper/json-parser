/*
 * jsonarray.cpp
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

#include "jsonarray.h"

#include <memory.h>
#include <cassert>
#include <memory>

namespace json {


Array::Array(Uint initial_size)
{
    reserve(initial_size);
}


Array::Array(json::const_iterator first, json::const_iterator last)
{
    insert(0, first, last);
}


Array::Array(std::initializer_list<Value> const& list)
{
    insert(0, list.begin(), list.end());
}


Array::Array(Array const& other)
{
    assign(other);
}


Array::Array(Array&& other)
{
    swap(other);
}


Array::~Array()
{
    std::_Destroy(begin(), end());

    if (m_data)
        ::free(m_data);

    m_data = null;
    m_num_items = 0;
    m_allocated_size = 0;
}


Array& Array::operator=(Array const& other)
{
    assign(other);
    return *this;
}


Array& Array::operator=(Array&& other)
{
    swap(other);
    return *this;
}


void Array::assign(Array const& other)
{
    Array tmp(other.reserved());
    tmp.append(other.begin(), other.end());
    swap(tmp);
}


void Array::assign(json::const_iterator first, json::const_iterator last)
{
    Array tmp(first, last);
    swap(tmp);
}


void Array::swap(Array& other)
{
    std::swap(m_data, other.m_data);
    std::swap(m_num_items, other.m_num_items);
    std::swap(m_allocated_size, other.m_allocated_size);
}


void Array::clear()
{
    std::_Destroy(begin(), end());
    m_num_items = 0;
}


Uint Array::numItems() const
{
    return m_num_items;
}


Uint Array::reserved() const
{
    return m_allocated_size;
}


Value* Array::data() const
{
    return m_data;
}


void Array::reserve(Uint new_size)
{
    if (new_size == reserved() and m_data != null)
        return;

    Uint num_elements = numItems();

    if (new_size < numItems())
    {
        std::_Destroy(m_data + new_size, m_data + numItems());
        num_elements = new_size;
    }

    void* mem = ::realloc(m_data, sizeof(Value) * (new_size + 1));

    assert(mem != null);

    m_data = static_cast<Value*>(mem);

    if (new_size > numItems())
        ::memset(m_data + numItems(), 0, sizeof(Value) * (new_size - numItems()));

    m_allocated_size = new_size;
    m_num_items = num_elements;
}


void Array::resize(Uint new_size)
{
    Uint new_alloc = (new_size * 3) / 2 + 1;

    if (new_size < numItems())
    {
        std::_Destroy(m_data + new_size, m_data + numItems());

        if (new_alloc < reserved())
            reserve(new_alloc);
    }
    else if (new_size > reserved())
        reserve(new_alloc);

    m_num_items = new_size;
}


void Array::insert(Uint index, Value const& value)
{
    insert(index, &value, (&value) + 1);
}


void Array::insert(Uint position, json::const_iterator first, json::const_iterator last)
{
    Uint num_elements = Value::distance(first, last);
    Uint old_size = numItems();
    Uint new_size = num_elements + old_size;
    Uint index = position;

    resize(new_size);
    Value* current = data() + index;

    ::memmove(current + num_elements, current, sizeof(Value) * (old_size - index));
    ::memset(current, 0, sizeof(Value) * num_elements);

    while (first < last)
    {
        if (first->isUsed())
        {
            current->assign(*first);
            ++current;
        }

        ++first;
    }
}


void Array::append(Value const& value)
{
    insert(numItems(), value);
}


void Array::append(json::const_iterator first, json::const_iterator last)
{
    insert(numItems(), first, last);
}


void Array::prepend(Value const& value)
{
    insert(0, value);
}


void Array::prepend(json::const_iterator first, json::const_iterator last)
{
    insert(0, first, last);
}


void Array::remove(Uint index)
{
    if (index >= numItems() or numItems() == 0)
        return;

    Value* value = data() + index;

    value->~Value();
    ::memmove(value, value + 1, sizeof(Value) * (numItems() - 1 - index));
    resize(numItems() - 1);
}


Value& Array::operator[](Uint index)
{
    if (index < numItems())
        return m_data[index];

    if (index >= reserved())
        reserve(index + 1);

    if (index >= numItems())
        m_num_items = index + 1;

    return m_data[index];
}


Value const& Array::operator[](Uint index) const
{
    if (index < numItems())
        return m_data[index];
    else
        return *(m_data + numItems());
}


json::iterator Array::begin()
{
    return data();
}


json::const_iterator Array::begin() const
{
    return data();
}


json::iterator Array::end()
{
    return data() + numItems();
}


json::const_iterator Array::end() const
{
    return data() + numItems();
}


}  // namespace json


