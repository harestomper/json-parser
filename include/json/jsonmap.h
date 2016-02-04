/*
 * jsonmap.h
 *
 *  Created on: 7 янв. 2016 г.
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

#ifndef _JSON_JSONMAP_H_
#define _JSON_JSONMAP_H_


#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "json.h"

namespace json {
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class Map : protected MapBase
{
public:
    static const Uint bucket_size = 6;

    typedef json::iterator  iterator;
    typedef json::const_iterator    const_iterator;

    Map(Uint initial_buckets_count = 0);
    Map(std::initializer_list<std::pair<char const*, Value>> const& list);
    Map(Map&& other);
    Map(Map const& other);
    ~Map();

    Map& operator=(Map&& other);
    Map& operator=(Map const& other);

    void clear();
    void swap(Map& other);
    void assign(Map const& other);

    Uint numItems() const;
    Uint capacity() const;
    Uint numBuckets() const;

    bool hasKey(char const* key) const;
    void getKeys(std::vector<char const*>* result) const;
    char const* getKey(json::const_iterator iter) const;

    bool remove(char const* key);
    void replace(char const* key, Value const& value);
    void insert(char const* key, Value const& value);
    void insert(std::string const& key, Value const& value);

    Value& operator[](char const* key);
    Value& operator[](std::string const& key);
    Value const& operator[](char const* key) const;
    Value const& operator[](std::string const& key) const;

    json::iterator begin();
    json::iterator end();
    json::const_iterator begin() const;
    json::const_iterator end() const;

private:
    void setBucketsCount(Uint new_buckets_count);
    Value* insertRaw(Uint hash, char const* key, Value* value);
    Value const* find(char const* key) const;
    Value const* find(Uint hash, char const* key) const;

    void hashCodeToIndex(Uint hash_key, Uint* first, Uint *last) const;

    void setKey(Uint index, char const* key, Uint code);
    void clear(Uint index);
    bool itemEqual(Uint index, Uint hash, char const* key) const;
    bool itemIsUsed(Uint index) const;

    static Uint getHashCode(char const* str);
    static bool strEquals(char const* s1, char const* s2);
    static Uint getIndex(Uint bucket_index, Uint item_index);
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}  // namespace json



#endif /* _JSON_JSONMAP_H_ */
