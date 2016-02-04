/*
 * jsonvalue.cpp
 *
 *  Created on: 10 янв. 2016 г.
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

#include "jsonvalue.h"

#include <memory.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <cstdio>
#include <typeinfo>

namespace json {

#define INDENT 2
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define return_if_fail(expr, iter, ...) do { \
        if (not (expr)) \
        { \
            char __b[BUFSIZE]; \
            int len = ::snprintf(__b, BUFSIZE, "%s:%s:%i:'%s', failure. ", __FILE__, __func__, __LINE__, #expr); \
            ::snprintf(__b + len, BUFSIZE - len, " " __VA_ARGS__); \
            iter.appendReason(__b); \
            return; \
        } } while (0)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define return_val_if_fail(expr, rval, iter, ...) do { \
        if (not (expr)) \
        { \
            char __b[BUFSIZE]; \
            int len = ::snprintf(__b, BUFSIZE, "%s:%s:%i:'%s', failure. ", __FILE__, __func__, __LINE__, #expr); \
            ::snprintf(__b + len, BUFSIZE - len, " " __VA_ARGS__); \
            iter.appendReason(__b); \
            return (rval); \
        } } while (0)
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator::StateIterator()
{
    init();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator::StateIterator(char const* first_, char const* last_, char const* cur) :
        first(first_),
        last(last_),
        current(cur ? cur : first_)
{
    init();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator::StateIterator(StateIterator const& other)
{
    init();
    assign(other);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator::~StateIterator()
{
    this->unref();
    this->first = this->last = this->current = null;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator& Value::StateIterator::operator=(char const* other)
{
    this->current = other;
    return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator& Value::StateIterator::operator+=(int nth)
{
    this->increase(nth);
    return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::StateIterator::appendReason(char const* reason)
{
    if (this->rf == null)
        this->ref();

    void* mem = ::realloc(this->rf, sizeof(*this->rf) * (this->num_reasons + 3));
    this->rf = static_cast<int*>(mem);
    this->reasons = static_cast<char**>(static_cast<void*>(this->rf + 1));
    this->reasons[this->num_reasons] = reason ? ::strdup(reason) : null;
    this->num_reasons += 1;
    this->reasons[this->num_reasons] = null;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator::operator bool() const
{
    return (this->current and *this->current and this->current < this->last);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::StateIterator::operator!() const
{
    bool result = (*this);
    return (not result);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline char Value::StateIterator::operator*() const
{
    return (this->current ? *this->current : 0);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::StateIterator& Value::StateIterator::operator++()
{
    this->increase(1);
    return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::StateIterator::increase(int count)
{
    if (this->current >= this->last or *this->current == 0 or count == 0)
        return;

    do {
        unsigned uc = *this->current;

        if      ((uc & 0xfc) == 0xfc)
            this->current += 6;
        else if ((uc & 0xf8) == 0xf8)
            this->current += 5;
        else if ((uc & 0xf0) == 0xf0)
            this->current += 4;
        else if ((uc & 0xe0) == 0xe0)
            this->current += 3;
        else if ((uc & 0xc0) == 0xc0)
            this->current += 2;
        else if ((uc & 0x80) == 0x80)
            while (this->current < this->last and (*this->current & 0x80) == 0x80)
                ++this->current;
        else
            ++this->current;
    } while (--count > 0 and this->current < this->last and *this->current != 0);

    return;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline char Value::StateIterator::operator[](int ix)
{
    return this->current[ix];
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline int Value::StateIterator::index() const
{
    return this->current - this->first;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::StateIterator::assign(StateIterator const& other)
{
    this->unref();
    this->first = other.first;
    this->last = other.last;
    this->current = other.current;
    this->reasons = other.reasons;
    this->rf = other.rf;
    this->num_reasons = other.num_reasons;
    this->ref();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline int Value::StateIterator::ref()
{
    if (this->rf)
    {
        ++(*this->rf);
        return *this->rf;
    }

    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline int Value::StateIterator::unref()
{
    if (this->rf)
    {
        --(*this->rf);
        if (*this->rf <= 0)
        {
            for (Uint i = 0; i < this->num_reasons; ++i)
            {
                ::free(this->reasons[i]);
                this->reasons[i] = null;
            }

            ::free(this->rf);
            this->num_reasons = 0;
            this->reasons = null;
            return 0;
        }

        return *this->rf;
    }
    return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::StateIterator::init()
{
    void* mem = ::malloc(sizeof(int));
    this->rf = static_cast<int*>(mem);
    *this->rf = 1;
    this->reasons = static_cast<char**>(static_cast<void*>(this->rf + 1));
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(Type tp) :
        m_type(tp)
{
    switch (tp)
    {
        case Type::typeBoolean:
            construct<bool>(false);
            break;
        case Type::typeInteger:
            construct<Integer>(0);
            break;
        case Type::typeDouble:
            construct<double>(0.0);
            break;
        case Type::typeString:
            construct<std::string>();
            break;
        case Type::typeArray:
            construct<Array>();
            break;
        case Type::typeMap:
            construct<Map>();
            break;
        default:
            break;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(bool value) :
        m_type(Type::typeBoolean)
{
    construct<bool>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(long int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(long long int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(unsigned int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(unsigned long int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(unsigned long long int value) :
        m_type(Type::typeInteger)
{
    construct<Integer>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(char const* value) :
        m_type(Type::typeString)
{
    construct<std::string>(value ? value : "");
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(std::string const& value) :
        m_type(Type::typeString)
{
    construct<std::string>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(float value) :
        m_type(Type::typeDouble)
{
    construct<double>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(double value) :
        m_type(Type::typeDouble)
{
    construct<double>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(Array const& value) :
        m_type(Type::typeArray)
{
    construct<Array>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(std::initializer_list<Value> const& list) :
        m_type(Type::typeArray)
{
    construct<Array>(list);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(Map const& value) :
        m_type(Type::typeMap)
{
    construct<Map>(value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(std::initializer_list<std::pair<char const*, Value>> const& list) :
        m_type(Type::typeMap)
{
    construct<Map>(list);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::~Value()
{
    switch (type())
    {
        case Type::typeBoolean:
            destruct<bool>();
            break;
        case Type::typeInteger:
            destruct<Integer>();
            break;
        case Type::typeDouble:
            destruct<double>();
            break;
        case Type::typeString:
            destruct<std::string>();
            break;
        case Type::typeArray:
            destruct<Array>();
            break;
        case Type::typeMap:
            destruct<Map>();
            break;
        default:
            break;
    }

    m_type = Type::untyped;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(Value const& other)
{
    assign(other);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::Value(Value&& other)
{
    this->swap(other);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value& Value::operator=(Value const& other)
{
    assign(other);
    return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value& Value::operator=(Value&& other)
{
    this->swap(other);
    return *this;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline Value::Type Value::type() const
{
    return m_type;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::empty() const
{
    return type() == Type::untyped;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::isUsed() const
{
    return m_type != Type::untyped;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isNull() const
{
    return type() == Type::typeNull;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isBoolean() const
{
    return type() == Type::typeBoolean;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isInteger() const
{
    return type() == Type::typeInteger;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isDouble() const
{
    return type() == Type::typeDouble;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isString() const
{
    return type() == Type::typeString;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isArray() const
{
    return type() == Type::typeArray;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::isMap() const
{
    return type() == Type::typeMap;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Uint Value::size() const
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>().numItems();
        case Type::typeMap:
            return as<Map>().numItems();
        default:
            return 0;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::hasKey(char const* key) const
{
    switch (type())
    {
        case Type::typeMap:
            return as<Map>().hasKey(key);
        default:
            return false;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::hasKey(std::string const& key) const
{
    return hasKey(key.c_str());
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator bool() const
{
    return asBoolean();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator long int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator long long int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator unsigned int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator unsigned long int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator unsigned long long int() const
{
    return asInteger();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator float() const
{
    return asDouble();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator double() const
{
    return asDouble();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator char*() const
{
    return asCString();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator std::string() const
{
    return asString();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator Array() const
{
    return asArray();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value::operator Map() const
{
    return asMap();
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::asBoolean() const
{
    switch (type())
    {
        case Type::typeBoolean:
            return as<bool>();
        case Type::typeInteger:
            return as<Integer>();
        case Type::typeDouble:
            return as<double>();
        case Type::typeString:
            return (as<std::string>().size() > 0);
        case Type::typeMap:
        case Type::typeArray:
            return (size() > 0);
        case Type::untyped:
        default:
            return false;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Integer Value::asInteger() const
{
    switch (type())
    {
        case Type::typeBoolean:
            return (as<bool>() ? 1 : 0);
        case Type::typeInteger:
            return as<Integer>();
        case Type::typeDouble:
            return as<double>();
        case Type::typeString:
            return as<std::string>().size();
        case Type::typeMap:
        case Type::typeArray:
            return this->size();
        case Type::untyped:
        default:
            return 0;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
double  Value::asDouble() const
{
    switch (type())
    {
        case Type::typeBoolean:
            return as<bool>() ? 1.0 : 0.0;
        case Type::typeInteger:
            return as<Integer>();
        case Type::typeDouble:
            return as<double>();
        default:
            return 0.0;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::string Value::asString() const
{
    std::string result;

    dumpInternal(&result, false, false, 0);

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char* Value::asCString() const
{
    std::string result;

    if (dumpInternal(&result, false, false, 0))
    {
        void* mem = ::malloc(sizeof(char) * (result.size() + 1));
        check_and_return_val(mem != null, null);
        char* retval = static_cast<char*>(mem);
        ::memcpy(retval, result.c_str(), sizeof(char) * result.size());
        retval[result.size()] = 0;
        return retval;
    }

    return null;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Array Value::asArray() const
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>();
        default:
            return Array();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Map Value::asMap() const
{
    switch (type())
    {
        case Type::typeMap:
            return as<Map>();
        default:
            return Map();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::insert(char const* key, Value const& value)
{
    if (not isMap())
        return;

    as<Map>().insert(key, value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::insert(std::string const& key, Value const& value)
{
    if (not isMap())
        return;

    as<Map>().insert(key, value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::insert(Uint ix, Value const& value)
{
    if (not isArray())
        return;

    as<Array>().insert(ix, value);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::remove(char const* key)
{
    if (not isMap())
        return;

    as<Map>().remove(key);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::remove(std::string const& key)
{
    if (not isMap())
        return;

    as<Map>().remove(key.c_str());
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::remove(Uint ix)
{
    if (not isArray())
        return;

    as<Array>().remove(ix);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::get(char const* key, Value const& result)
{
    if (isMap() and as<Map>().hasKey(key))
    {
        result = as<Map>()[key];
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::get(std::string const& key, Value const& result)
{
    if (isMap() and as<Map>().hasKey(key.c_str()))
    {
        result = as<Map>()[key];
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::get(Uint ix, Value const& result)
{
    if (isArray() and ix < size())
    {
        result = as<Array>()[ix];
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value& Value::operator[](char const* key)
{
    switch (type())
    {
        case Type::typeMap:
            return as<Map>()[key];
        case Type::typeArray:
            return *as<Array>().end();
        default:
            throw std::bad_cast();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value& Value::operator[](std::string const& key)
{
    return (*this)[key.c_str()];
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value& Value::operator[](int ix)
{
    switch (type())
    {
        case Type::typeMap:
            return *as<Map>().end();
        case Type::typeArray:
            return as<Array>()[ix];
        default:
            throw std::bad_cast();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value const& Value::operator[](char const* key) const
{
    switch (type())
    {
        case Type::typeMap:
            return as<Map>()[key];
        case Type::typeArray:
            return *as<Array>().end();
        default:
            throw std::bad_cast();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value const& Value::operator[](std::string const& key) const
{
    return (*this)[key.c_str()];
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Value const& Value::operator[](int ix) const
{
    switch (type())
    {
        case Type::typeMap:
            return *as<Map>().end();
        case Type::typeArray:
            return as<Array>()[ix];
        default:
            throw std::bad_cast();
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char const* Value::getKey(const_iterator iter) const
{
    switch (type())
    {
        case Type::typeMap:
            return as<Map>().getKey(iter);
        default:
            return null;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::assign(Value const& other)
{
    Value dummy(other.type());

    switch (other.type())
    {
        case Type::typeMap:
            dummy.as<Map>().assign(other.as<Map>());
            break;

        case Type::typeArray:
            dummy.as<Array>().assign(other.as<Array>());
            break;

        case Type::typeString:
            dummy.as<std::string>().assign(other.as<std::string>());
            break;

        default:
            dummy.m_data = other.m_data;
    }

    swap(dummy);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::swap(Value& other)
{
    std::swap(m_data, other.m_data);
    std::swap(m_type, other.m_type);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Value::clear()
{
    Value dummy;
    swap(dummy);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
json::iterator Value::begin()
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>().begin();
        case Type::typeMap:
            return as<Map>().begin();
        default:
            return this;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
json::iterator Value::end()
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>().end();
        case Type::typeMap:
            return as<Map>().end();
        default:
            return this;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
json::const_iterator Value::begin() const
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>().begin();
        case Type::typeMap:
            return as<Map>().begin();
        default:
            return this;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
json::const_iterator Value::end() const
{
    switch (type())
    {
        case Type::typeArray:
            return as<Array>().end();
        case Type::typeMap:
            return as<Map>().end();
        default:
            return this;
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Uint Value::distance(const_iterator first, const_iterator last)
{
    int result = 0;

    while (first < last)
    {
        result += first->isUsed();
        ++first;
    }

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Uint Value::distance(iterator first, iterator last)
{
    int result = 0;

    while (first < last)
    {
        result += first->isUsed();
        ++first;
    }

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::parseStream(FILE* fd)
{
    check_and_return_val(fd != null, false);

    int const block_size = 1024;
    int ret_value = 0;
    char block[block_size];
    char *result_data = null;
    Uint length = 0;
    bool result = false;

    do {
        ret_value = ::fread(block, sizeof(char), block_size, fd);
        result_data = appendData(result_data, length, block, ret_value);
    } while (ret_value == block_size and result_data);

    check_and_return_val(result_data != null, false);

    result = parseData(result_data, result_data + length);
    ::free(result_data);

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::parseFile(const char* filename)
{
    check_and_return_val(filename != null and *filename != '\0', false);

    FILE* fd = ::fopen(filename, "rb");
    bool result = parseStream(fd);

    if (fd)
        ::fclose(fd);

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::parseFile(std::string const& filename)
{
    return parseFile(filename.c_str());
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::parseData(char const* first, char const* last)
{
    Value val;
    StateIterator iter(first, last);

    if (val.parseValue(iter))
    {
        (*this) = val;
        return true;
    }
    else
    {
        char** it = iter.reasons;

        while (*it)
        {
            printf("%s\n", *it);
            ++it;
        }

        int const block_size = 40;
        char block[block_size + 1];
        char const* start = iter.current;
        char const* end = std::min(start + block_size, iter.last);
        ::memcpy(block, start, sizeof(char) * (end - start));
        block[end - start] = 0;
        char* p = block;

        while (*p)
        {
            switch (*p)
            {
                case '\n':
                case '\r':
                case '\v':
                case '\t':
                    *p = ' ';
                    break;
                default:
                    break;
            }
            ++p;
        }

        ::fprintf(::stderr, "Syntax error at position %u:\n'%s'\n", iter.index(), block);
    }

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::parseString(std::string const& data)
{
    return parseData(data.c_str(), data.c_str() + data.size());
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::saveToStream(FILE* fd, bool pretty_print) const
{
    check_and_return_val(fd != null, false);

    std::string result;

    check_and_return_val(dumpInternal(&result, pretty_print, false, 0), false);
    check_and_return_val(result.size() > 0, false);

    int retval = ::fwrite(result.data(), sizeof(char), result.size(), fd);

    check_and_return_val(retval >= 0, false, "%i:%s", errno, strerror(errno));

    return true;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::saveToFile(char const* filename, bool pretty_print) const
{
    check_and_return_val(filename != null and *filename != '\0', false);

    FILE* fd = ::fopen(filename, "wb");
    bool result = saveToStream(fd, pretty_print);

    if (fd)
        ::fclose(fd);

    return result;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::saveToFile(std::string const& filename, bool pretty_print) const
{
    return saveToFile(filename.c_str(), pretty_print);;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::saveToString(std::string* result, bool pretty_print) const
{
    return dumpInternal(result, pretty_print, false, 0);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char* Value::saveToData(int& length, bool pretty_print) const
{
    std::string result;
    char* retval = null;

    if (dumpInternal(&result, pretty_print, true, 0))
    {
        void* mem = ::malloc(sizeof(char) * (result.size() + 1));
        check_and_return_val(mem != null, null, "%i:%s", errno, ::strerror(errno));

        retval = static_cast<char*>(mem);
        ::memcpy(retval, result.c_str(), sizeof(char) * result.size());
        retval[result.size()] = 0;
        length = result.size();
    }

    return retval;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char* Value::appendData(char* data, Uint& length, char const* block, Uint count)
{
    if (count == 0)
        return data;

    void* mem = ::realloc(data, sizeof(*data) * (length + count + 1));

    check_and_return_val(mem != null, data, "%i:%s", errno, ::strerror(errno));

    data = static_cast<char*>(mem);
    ::memcpy(data + length, block, sizeof(*data) * count);
    length += count;
    data[length] = 0;
    return data;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::skipMultilineComment(StateIterator& iter)
{
    if (*iter != '/' or iter[1] != '*')
        return true;

    iter += 2;

    while (iter)
    {
        if (*iter == '\\')
            ++iter;

        if (iter and *iter == '*' and iter[1] == '/')
        {
            iter += 2;
            return (bool) iter;
        }

        ++iter;
    }

    return_val_if_fail(iter, false, iter, "Unexpected end of the comment block");

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::skipSinglelineComment(StateIterator& iter)
{
    if (not iter)
        return false;
    else if (iter[0] != '/' or iter[1] != '/')
        return true;

    iter += 2;

    while (iter)
    {
        if (*iter == '\\')
            ++iter;

        if (iter and *iter == '\n')
        {
            ++iter;
            return (bool) iter;
        }

        ++iter;
    }

    return_val_if_fail(iter, false, iter, "Unexpected end of the comment line");

    return true;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::skipCommentsAndSpaces(StateIterator& iter)
{
    while ((bool) iter)
    {
        switch (*iter)
        {
            case '\n' :
            case '\r' :
            case ' ' :
            case '\t' :
            case '\v' :
            case '\f' :
            case '\b' :
                ++iter;
                break;
            case '/' :
                return_val_if_fail(iter[1] == '/' or iter[1] == '*', false, iter, "Unexpected symbol");

                if (iter[1] == '*')
                    return_val_if_fail(skipMultilineComment(iter), false, iter);
                else if (iter[1] == '/')
                    return_val_if_fail(skipSinglelineComment(iter), false, iter);
                break;

            default:
                return (bool) iter;
        }
    };

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::parseString(StateIterator& iter, std::string& result)
{
    return_val_if_fail(skipCommentsAndSpaces(iter), false, iter);
    return_val_if_fail(*iter == '"', false, iter, "Expected '\"', but got '%c'", *iter);

    ++iter;

    while ((bool) iter)
    {
        return_val_if_fail(*iter != '\0', false, iter, "Unexpected end of data");

        switch (*iter)     /* switch 1 */
        {
            case '"' :
                ++iter;
                return (bool) iter;
            default:
                result.insert(result.end(), *iter);
                ++iter;
                break;

            case '\\' :
            {
                ++iter;

                switch (*iter) /* switch 2 */
                {
                    case 'u': // utf-16 and utf-32
                    {
                        char* end;
                        ++iter;
                        return_val_if_fail(iter, false, iter);

                        unsigned uc = std::strtoul(iter.current, &end, 16);

                        return_val_if_fail(uc <= 0x10ffff and end != null, false, iter, "Failed to parse hex string");

                        if (uc < 0x80)
                        {
                            result.append(1, uc);
                        }
                        else if (uc <= 0x07ff)
                        {
                            result.append(1, (0xc0 | ((uc >> 6) & 0x1f)));
                            result.append(1, (0x80 | ((uc >> 0) & 0x3f)));
                        }
                        else if (uc <= 0xffff)
                        {
                            result.append(1, (0xe0 | ((uc >> 12) & 0x0f)));
                            result.append(1, (0x80 | ((uc >>  6) & 0x3f)));
                            result.append(1, (0x80 | ((uc >>  0) & 0x3f)));
                        }
                        else
                        {
                            result.append(1, (0xf0 | ((uc >> 18) & 0x07)));
                            result.append(1, (0x80 | ((uc >> 12) & 0x3f)));
                            result.append(1, (0x80 | ((uc >>  6) & 0x3f)));
                            result.append(1, (0x80 | ((uc >>  0) & 0x3f)));
                        }

                        iter = end;

                        break;
                    }   /* end of case 'u' */

                    case 'x': // utf-8
                    {
                        char *end = null;
                        ++iter;
                        return_val_if_fail(iter, false, iter);

                        unsigned uc = std::strtoul(iter.current, &end, 16);

                        return_val_if_fail(end != null, false, iter, "Failed to parse hex string");

                        result.insert(result.end(), uc);
                        iter = end;
                        break;
                    }

                    case '"' :
                    case '/' :
                        result.insert(result.end(), *iter);
                        ++iter;
                        break;

                    case 't':
                    case 'f':
                    case 'n':
                    case 'v':
                    case 'r':
                    case 'b':
                    case '\\':
                    default:
                        result.insert(result.end(), '\\');
                        result.insert(result.end(), *iter);
                        ++iter;
                }   /* end of switch 2 */
                break;
            }   /* end of case '\\' */
        }   /* end of switch 1 */

    }   /* end of while */

    return_val_if_fail((bool) iter, false, iter, "Unexpected end of data");
    return_val_if_fail(*iter == '"', false, iter, "Unexpexted end of string. Expected '\"', but got '%c'", *iter);

    ++iter;

    return (bool) iter;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::strIsDouble(StateIterator iter)
{
    if (not skipCommentsAndSpaces(iter))
        return false;

    while (iter)
    {
        switch (*iter)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
            case '+':
                break;
            case 'e':
            case '.':
                return true;
            default:
                return false;
        }

        ++iter;
    }

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::parseArray(StateIterator& iter)
{
    return_val_if_fail(*iter == '[', false, iter, "Unexpected begin symbol for array");

    ++iter;

    while (iter and *iter != ']')
    {
        Value val;
        return_val_if_fail(val.parseValue(iter), false, iter);
        (*this)[size()] = val;

        if (*iter == ',')
            ++iter;

        return_val_if_fail(skipCommentsAndSpaces(iter), false, iter);
    }

    return_val_if_fail(iter and *iter == ']', false, iter, "Unexpected final symbol for array.");
    ++iter;

    return true;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::parseMap(StateIterator& iter)
{
    return_val_if_fail(iter and *iter == '{', false, iter, "Unexpected begin symbol for object");

    ++iter;

    while (iter and *iter != '}')
    {
        Value val;
        std::string key;

        return_val_if_fail(parseString(iter, key), false, iter);
        return_val_if_fail(skipCommentsAndSpaces(iter), false, iter);
        return_val_if_fail(*iter == ':', false, iter, "Unbound symbol. Expected ':', but got '%c'", *iter);
        return_val_if_fail((bool) (++iter), false, iter);
        return_val_if_fail(val.parseValue(iter), false, iter);

        (*this)[key] = val;

        if (*iter == ',')
            ++iter;

        return_val_if_fail(skipCommentsAndSpaces(iter), false, iter);
    }

    return_val_if_fail(iter, false, iter, "Unexpected end of data");
    return_val_if_fail(*iter == '}', false, iter, "Unexpected final symbol for object.");

    ++iter;

    return true;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline bool Value::parseValue(StateIterator& iter)
{
    while (iter)
    {
        return_val_if_fail(skipCommentsAndSpaces(iter), false, iter);

        switch (*iter)
        {
            case '{':
                (*this) = Value(Value::Type::typeMap);
                return parseMap(iter);

            case '[':
                (*this) = Value(Value::Type::typeArray);
                return parseArray(iter);

            case '"':
                (*this) = std::string();
                return parseString(iter, as<std::string>());

            case 't':
            {
                return_val_if_fail(::strncmp(iter.current, "true", 4) == 0, false, iter, "Unknown symbol 't'");
                (*this) = true;
                iter += 4;
                return (bool) iter;
            }

            case 'f':
            {
                return_val_if_fail(::strncmp(iter.current, "false", 5) == 0, false, iter, "Unknown symbol 'f'");
                (*this) = false;
                iter += 5;
                return (bool) iter;
            }

            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                char* end;

                if (::strncmp(iter.current, "0x", 2) == 0)
                    (*this) = ::strtoll(iter.current, &end, 16);
                else if (strIsDouble(iter))
                    (*this) = ::strtod(iter.current, &end);
                else
                    (*this) = ::strtoll(iter.current, &end, 10);

                return_val_if_fail(end != null, false, iter);
                iter = end;

                return (bool) iter;
            }

            case 'n':
            {
                return_val_if_fail(::strncmp(iter.current, "null", 4) == 0, false, iter, "Unknown symbol 'n'");
                (*this) = Value(Value::Type::typeNull);
                iter += 4;
                return (bool) iter;
            }

            case ',':
            default:
                ++iter;
                break;
        }

    }

    return false;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::dumpStringAsRaw(std::string* result) const
{
    std::string const& str = as<std::string>();

    result->reserve(result->size() + str.size());

    for (char c : str)
    {
        switch (c)
        {
            case '/'  :
                result->insert(result->end(), '\\');
                result->insert(result->end(), '/');
                break;

            case '\\' :
                result->insert(result->end(), '\\');
                result->insert(result->end(), '\\');
                break;

            case '"' :
                result->insert(result->end(), '\\');
                result->insert(result->end(), '"');
                break;

            case '\t' :
            case '\r' :
            case '\v' :
            case '\f' :
            case '\b' :
            default:
                result->insert(result->end(), c);
        }
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::dumpArray(std::string* result, bool pretty_print, bool as_raw, int indent) const
{
    const_iterator it = this->begin();
    const_iterator end = this->end();
    bool has_prev = false;
    std::string spaces(pretty_print ? (indent + 1) * INDENT : 0, ' ');
    std::string endline(pretty_print ? "\n" : "");

    if (size() and pretty_print)
    {
        result->append("\n");
        result->append(indent * INDENT, ' ');
    }

    result->append("[");

    while (it < end)
    {
        std::string str;

        if (it->dumpInternal(&str, pretty_print, as_raw, indent + 1))
        {
            char const* quotes = (it->isString() ? "\"" : "");

            if (has_prev)
                result->append(", ");

            if (not it->isMap() and not it->isArray())
                result->append(endline);

            result->append(spaces);
            result->append(quotes);
            result->append(str);
            result->append(quotes);

            has_prev = true;
        }

        ++it;
    }

    if (has_prev and pretty_print)
    {
        result->append("\n");
        result->append(indent * INDENT, ' ');
    }

    result->append("]");
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
inline void Value::dumpMap(std::string* result, bool pretty_print, bool as_raw, int indent) const
{
    const_iterator it = this->begin();
    const_iterator end = this->end();
    bool has_prev = false;
    std::string spaces(pretty_print ? (indent + 1) * INDENT : 0, ' ');
    std::string endline(pretty_print ? "\n" : "");

    if (indent and pretty_print)
    {
        result->append("\n");
        result->append(indent * INDENT, ' ');
    }

    result->append("{");

    while (it < end)
    {
        std::string str;
        char const* key;

        if (it->dumpInternal(&str, pretty_print, as_raw, indent + 1) and (key = as<Map>().getKey(it)))
        {
            char const* quotes = (it->isString() ? "\"" : "");

            if (has_prev)
                result->append(", ");

            result->append(endline);

            result->append(spaces);
            result->append("\"");
            result->append(key);
            result->append("\": ");
            result->append(quotes);
            result->append(str);
            result->append(quotes);
            has_prev = true;
        }

        ++it;
    }

    if (has_prev and pretty_print)
    {
        result->append("\n");
        result->append(indent * INDENT, ' ');
    }

    result->append("}");
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::dumpInternal(std::string* result, bool pretty_print, bool as_raw, int indent) const
{
    if (empty())
        return false;

    result->clear();

    switch (type())
    {
        case Type::typeNull:
            result->assign("null");
            break;
        case Type::typeBoolean:
            result->assign(asBoolean() ? "true" : "false");
            break;
        case Type::typeInteger:
            result->assign(std::to_string(asInteger()));
            break;
        case Type::typeDouble:
        {
            char buf[BUFSIZE];
            ::snprintf(buf, BUFSIZE, "%f", asDouble());
            result->assign(buf[0] ? buf : "0.0");
            break;
        }

        case Type::typeString:
        {
            if (not as_raw)
                result->assign(as<std::string>().c_str());
            else
            {
                std::string data;
                dumpStringAsRaw(&data);
                result->append(data);
            }

            break;
        }
        case Type::typeArray:
            dumpArray(result, pretty_print, as_raw, indent);
            break;

        case Type::typeMap:
            dumpMap(result, pretty_print, as_raw, indent);
            break;

        default:
            break;
    }

    return (result->size() > 0);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
}  // namespace json


