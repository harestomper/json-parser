/*
 * jsonvalue.h
 *
 *  Created on: 5 янв. 2016 г.
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

#ifndef _JSON_JSONVALUE_H_
#define _JSON_JSONVALUE_H_

#include <initializer_list>
//#include <memory>
#include <string>
//#include "jsonmap.h"
//#include "jsonarray.h"
//#include <utility>

#include "json.h"

namespace json {

template<size_t pS>
constexpr size_t static_max()
{
    return pS;
}

template<size_t pS, typename vT, typename ...Args>
constexpr size_t static_max()
{
    return static_max<sizeof(vT) < pS ? pS : sizeof(vT), Args...>();
}

struct Value
{
public:
    enum Type
    {
        untyped,
        typeNull,
        typeBoolean,
        typeInteger,
        typeDouble,
        typeString,
        typeArray,
        typeMap
    };

private:
    enum {
        holder_size = static_max<0, bool, Integer, double, std::string, ArrayBase, MapBase>()
    };

    using HolderType = typename std::aligned_storage<holder_size>::type;

    Type        m_type = Type::untyped;
    HolderType  m_data = {};

public:
    using iterator = Value*;
    using const_iterator = Value const*;

    Value(Type tp = Type::untyped);
    Value(bool value);
    Value(int value);
    Value(long int value);
    Value(long long int value);
    Value(unsigned int value);
    Value(unsigned long int value);
    Value(unsigned long long int value);
    Value(char const* value);
    Value(std::string const& value);
    Value(float value);
    Value(double value);
    Value(Array const& value);
    Value(std::initializer_list<Value> const& list);
    Value(Map const& value);
    Value(std::initializer_list<std::pair<char const*, Value>> const& list);
    ~Value();

    Value(Value const& other);
    Value(Value&& other);
    Value& operator=(Value const& other);
    Value& operator=(Value&& other);

    Type type() const;
    bool empty() const;
    bool isUsed() const;
    bool isNull() const;
    bool isBoolean() const;
    bool isInteger() const;
    bool isDouble() const;
    bool isString() const;
    bool isArray() const;
    bool isMap() const;

    Uint size() const;

    bool hasKey(char const* key) const;
    bool hasKey(std::string const& key) const;

    operator bool() const;
    operator int() const;
    operator long int() const;
    operator long long int() const;
    operator unsigned int() const;
    operator unsigned long int() const;
    operator unsigned long long int() const;
    operator float() const;
    operator double() const;
    operator char*() const;
    operator std::string() const;
    operator Array() const;
    operator Map() const;

    bool        asBoolean() const;
    Integer     asInteger() const;
    double      asDouble() const;
    std::string asString() const;
    char*       asCString() const;
    Array       asArray() const;
    Map         asMap() const;

    Value& operator[](char const* key);
    Value& operator[](std::string const& key);
    Value& operator[](int ix);

    Value const& operator[](char const* key) const;
    Value const& operator[](std::string const& key) const;
    Value const& operator[](int ix) const;

    char const* getKey(const_iterator iter) const;

    void assign(Value const& other);
    void swap(Value& other);
    void clear();

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

    static Uint distance(const_iterator first, const_iterator last);
    static Uint distance(iterator first, iterator last);

    bool parseStream(FILE* fd);
    bool parseFile(const char* filename);
    bool parseFile(std::string const& filename);
    bool parseData(char const* first, char const* last);
    bool parseString(std::string const& data);

    bool saveToStream(FILE* fd, bool pretty_print = false) const;
    bool saveToFile(char const* filename, bool pretty_print = false) const;
    bool saveToFile(std::string const& filename, bool pretty_print = false) const;
    bool saveToString(std::string* result, bool pretty_print = false) const;
    char* saveToData(int& length, bool pretty_print = false) const;

private:
    struct StateIterator
    {
        StateIterator();
        StateIterator(char const* first_, char const* last_, char const* cur = null);
        StateIterator(StateIterator const& other);
        ~StateIterator();

        StateIterator& operator=(char const* other);
        StateIterator& operator+=(int nth);
        void appendReason(char const* msg);
        operator bool() const;
        bool operator!() const;
        char operator*() const;
        StateIterator& operator++();
        void increase(int count);
        char operator[](int ix);
        int index() const;

        void assign(StateIterator const& other);

    private:
        int ref();
        int unref();
        void init();

    public:
        char const* first = null;
        char const* last = null;
        char const* current = null;
        char** reasons = null;
        Uint num_reasons = 0;

    private:
        int* rf = null;
    };

private:
    template<typename rT>
    rT& as()
    {   return *reinterpret_cast<rT*>(&m_data); }

    template<typename rT>
    rT const& as() const
    {   return *reinterpret_cast<rT const*>(&m_data); }

    template<typename vT, typename ...Args>
    void construct(Args&&... args)
    {   ::new(reinterpret_cast<vT*>(&m_data)) vT(std::forward<Args>(args)...); }

    template<typename vT>
    void destruct()
    {   reinterpret_cast<vT*>(&m_data)->~vT(); }

private:
    static char* appendData(char* data, Uint& length, char const* block, Uint count);
    static bool skipCommentsAndSpaces(StateIterator& iter);
    static bool skipMultilineComment(StateIterator& iter);
    static bool skipSinglelineComment(StateIterator& iter);
    static bool parseString(StateIterator& iter, std::string& result);
    static bool strIsDouble(StateIterator iter);

private:
    bool parseArray(StateIterator& iter);
    bool parseMap(StateIterator& iter);
    bool parseValue(StateIterator& iter);

    void dumpStringAsRaw(std::string* result) const;
    void dumpArray(std::string* result, bool pretty_print, bool as_raw, int indent) const;
    void dumpMap(std::string* result, bool pretty_print, bool as_raw, int indent) const;
    bool dumpInternal(std::string* result, bool pretty_print, bool as_raw, int indent) const;
};

}  // namespace json


#endif /* _JSON_JSONVALUE_H_ */
