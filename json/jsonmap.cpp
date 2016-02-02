/*
 * jsonmap.cpp
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


#include "json/jsonmap.h"

#include <memory.h>
#include <stdlib.h>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>


namespace json {

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Map::Map(Uint initial_buckets_count)
{
    setBucketsCount(initial_buckets_count ? initial_buckets_count : 1);
}


Map::Map(std::initializer_list<std::pair<char const*, Value>> const& list)
{
    setBucketsCount(10);

    for (auto it : list)
        (*this)[it.first] = it.second;
}


Map::Map(Map&& other)
{
    swap(other);
}


Map::Map(Map const& other)
{
    assign(other);
}


Map::~Map()
{
    Uint ix;
    Uint last = capacity();

    for (ix = 0; ix < last; ++ix)
        clear(ix);

    if (m_keys)
        ::free(m_keys);

    if (m_codes)
        ::free(m_codes);

    if (m_values)
        ::free(m_values);

    m_keys = null;
    m_codes = null;
    m_values = null;
    m_num_items = 0;
    m_num_buckets = 0;
    m_capacity = 0;
}


Map& Map::operator=(Map&& other)
{
    swap(other);
    return *this;
}


Map& Map::operator=(Map const& other)
{
    assign(other);
    return *this;
}


void Map::clear()
{
    Map tmp;
    swap(tmp);
}


void Map::swap(Map& other)
{
    std::swap(m_values, other.m_values);
    std::swap(m_keys, other.m_keys);
    std::swap(m_codes, other.m_codes);
    std::swap(m_capacity, other.m_capacity);
    std::swap(m_num_buckets, other.m_num_buckets);
    std::swap(m_num_items, other.m_num_items);
}


void Map::assign(Map const& other)
{
    Map tmp(other.capacity());
    Uint ix;

    for (ix = 0; ix < other.capacity(); ++ix)
    {
        if (other.itemIsUsed(ix))
        {
            tmp.setKey(ix, other.m_keys[ix], other.m_codes[ix]);
            tmp.m_values[ix] = other.m_values[ix];
        }
    }

    tmp.m_num_items = other.numItems();
    swap(tmp);
}


Uint Map::numItems() const
{
    return m_num_items;
}


Uint Map::capacity() const
{
    return m_capacity;
}


Uint Map::numBuckets() const
{
    return this->m_num_buckets;
}


bool Map::hasKey(char const* key) const
{
    return (find(key) != null);
}


void Map::getKeys(std::vector<char const*>* result) const
{
    Uint first, last;
    char const* str;

    last = capacity();

    result->clear();
    result->resize(numItems());

    for (first = 0; first < last; ++first)
    {
        if (m_keys[first])
        {
            str = m_keys[first];
            result->push_back(str);
        }
    }
}


char const* Map::getKey(json::const_iterator iter) const
{
    Uint index = iter - begin();
    return (index < capacity() ? m_keys[index] : null);
}


bool Map::remove(char const* key)
{
    Value* value = const_cast<Value*>(find(key));

    if (value)
    {
        Uint index = value - m_values;
        clear(index);
        --m_num_items;
    }

    return (value != null);
}


void Map::replace(char const* key, Value const& value)
{
    Uint hash_key = getHashCode(key);
    Value* found = const_cast<Value*>(find(hash_key, key));

    if (found)
        *found = value;
    else
    {
        Value copy(value);
        insertRaw(hash_key, key, &copy);
    }
}


void Map::insert(char const* key, Value const& value)
{
    Value copy(value);
    insertRaw(getHashCode(key), key, &copy);
}


void Map::insert(std::string const& key, Value const& value)
{
    insert(key.c_str(), value);
}


void Map::setBucketsCount(Uint new_buckets_count)
{
    void* mem;
    Uint real_new_size;
    bool force_reset = (m_values == null);

    if (new_buckets_count == numBuckets() and not force_reset)
        return;

    if (new_buckets_count < numBuckets())
    {
        Uint index = numBuckets() * Map::bucket_size;

        while (index < capacity())
        {
            m_num_items -= itemIsUsed(index);
            clear(index);
        }
    }

    real_new_size = new_buckets_count * Map::bucket_size + 1;

    mem = ::realloc(m_values, sizeof(*m_values) * real_new_size);
    assert(mem != null);
    m_values = static_cast<Value*>(mem);

    mem = ::realloc(m_keys, sizeof(*m_keys) * real_new_size);
    assert(mem != null);
    m_keys = static_cast<char**>(mem);

    mem = ::realloc(m_codes, sizeof(*m_codes) * real_new_size);
    assert(mem != null);
    m_codes = static_cast<Uint*>(mem);

    if (new_buckets_count > numBuckets() or force_reset)
    {
        Uint rest = real_new_size - capacity();
        ::memset(m_values + capacity(), 0, sizeof(*m_values) * rest);
        ::memset(m_keys + capacity(), 0, sizeof(*m_keys) * rest);
        ::memset(m_codes + capacity(), 0, sizeof(*m_codes) * rest);
    }

    m_num_buckets = new_buckets_count;
    m_capacity = real_new_size - 1;
}


Value& Map::operator[](char const* key)
{
    Uint hash_key = getHashCode(key);
    Value const* value = find(hash_key, key);

    if (value)
        return *const_cast<Value*>(value);
    else
    {
        Value dummy;
        return *insertRaw(hash_key, key, &dummy);
    }
}


Value& Map::operator[](std::string const& key)
{
    return (*this)[key.c_str()];
}


Value const& Map::operator[](char const* key) const
{
    Uint hash_key = getHashCode(key);
    Value const* value = find(hash_key, key);

    if (value)
        return *const_cast<Value*>(value);
    else
        return m_values[capacity()];
}


Value const& Map::operator[](std::string const& key) const
{
    return (*this)[key.c_str()];
}


json::iterator Map::begin()
{
    return m_values;
}


json::iterator Map::end()
{
    return m_values + capacity();
}


json::const_iterator Map::begin() const
{
    return m_values;
}


json::const_iterator Map::end() const
{
    return m_values + capacity();
}


inline Value* Map::insertRaw(Uint hash_key, char const* key, Value* value)
{
    Uint index, last;

    hashCodeToIndex(hash_key, &index, &last);

    while (index < last)
    {
        if (not itemIsUsed(index))
        {
            setKey(index, key, hash_key);
            m_values[index].swap(*value);
            ++m_num_items;

            return m_values + index;
        }

        ++index;
    }

    Uint num_buckets = numBuckets();
    Uint bucket_to, item_to, item_to_last, item_from, bucket_from;

    setBucketsCount((num_buckets * 3) / 2 + 1);

    for (bucket_from = 0; bucket_from < num_buckets; ++bucket_from)
    {
        for (item_from = 0; item_from < Map::bucket_size; ++item_from)
        {
            index = bucket_from * Map::bucket_size + item_from;

            if (not itemIsUsed(index))
                continue;

            bucket_to = m_codes[index] % numBuckets();

            if (bucket_to == bucket_from)
                continue;

            item_to = bucket_to * Map::bucket_size;
            item_to_last = item_to + Map::bucket_size;

            for (; item_to < item_to_last; ++item_to)
            {
                if (not itemIsUsed(item_to))
                {
                    std::swap(m_codes[item_from], m_codes[item_to]);
                    std::swap(m_keys[item_from], m_keys[item_to]);
                    m_values[item_from].swap(m_values[item_to]);
                    break;
                }
            }

            if (item_to >= item_to_last)
                return insertRaw(hash_key, key, value);
        }
    }

    return insertRaw(hash_key, key, value);
}


inline Value const* Map::find(char const* key) const
{
    return find(getHashCode(key), key);
}


inline Value const* Map::find(Uint hash_key, char const* key) const
{
    Uint current, last;

    hashCodeToIndex(hash_key, &current, &last);

    while (current < last)
    {
        if (itemEqual(current, hash_key, key))
            return &m_values[current];
        ++current;
    }

    return null;
}


inline void Map::hashCodeToIndex(Uint hash_key, Uint* first, Uint *last) const
{
    Uint start = 0;
    Uint end = 0;

    if (numBuckets())
    {
        start = (hash_key % numBuckets()) * Map::bucket_size;
        end = start + Map::bucket_size;

    }

    if (first)
        *first = start;

    if (last)
        *last = end;
}


void Map::setKey(Uint index, char const* key, Uint code)
{
    char* str = m_keys[index];

    if (str)
        ::free(str);

    m_keys[index] = null;
    m_codes[index] = 0;

    if (key)
    {
        int len = ::strlen(key);
        void* mem = ::malloc(sizeof(*str) * (len + 1));

        assert(mem != null);

        str = static_cast<char*>(mem);
        ::memcpy(str, key, sizeof(*str) * len);
        str[len] = 0;

        m_keys[index] = str;
        m_codes[index] = code ? code : getHashCode(key);
    }
}


inline void Map::clear(Uint index)
{
    if (m_keys[index])
    {
        setKey(index, null, 0);
        m_values[index].~Value();
    }
}


inline bool Map::itemEqual(Uint index, Uint hash_key, char const* key) const
{
    return (m_codes[index] == hash_key and strEquals(m_keys[index], key));
}


inline bool Map::itemIsUsed(Uint index) const
{
    return (index < capacity() and m_keys[index] != null);
}


inline Uint Map::getHashCode(char const* str)
{
    Uint hash_key = 0;
    char const* p = str;

    while (*p)
        hash_key = ((hash_key << 5) + hash_key) + *p++;

    return hash_key;
}


inline bool Map::strEquals(char const* s1, char const* s2)
{
    char const* str1 = s1 == null ? "" : s1;
    char const* str2 = s2 == null ? "" : s2;

    return (::strcmp(str1, str2) == 0);
}


inline Uint Map::getIndex(Uint bucket_index, Uint item_index)
{
    return bucket_index * Map::bucket_size + item_index;
}


}  // namespace json


