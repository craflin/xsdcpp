
#pragma once

#ifndef XSDCPP_H
#define XSDCPP_H

#include <cstdint>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace xsd {

typedef std::string string;

template <typename T>
using vector = std::vector<T>;
/*
template <typename T>
class basic_optional
{
public:
    basic_optional()
        : _valid(false)
    {
    }
    basic_optional(const basic_optional& other)
        : _valid(other._valid)
    {
        if (_valid)
            ::new (&_data) T(*(T*)&other._data);
    }

    basic_optional(basic_optional&& other)
        : _valid(other._valid)
    {
        if (_valid)
        {
            ::new (&_data) T(std::move(*(T*)&other._data));
            ((T*)&other._data)->~T();
            other._valid = false;
        }
    }

    basic_optional(const T& other)
        : _valid(true)
    {
        ::new (&_data) T(other);
    }

    basic_optional(T&& other)
        : _valid(true)
    {
        ::new (&_data) T(std::move(other));
    }

    ~basic_optional()
    {
        if (_valid)
            ((T*)&_data)->~T();
    }

    basic_optional& operator=(const basic_optional& other)
    {
        if (_valid != other._valid)
        {
            if (_valid)
            {
                ((T*)&_data)->~T();
                _valid = false;
            }
            else
            {
                ::new (&_data) T(*(T*)&other._data);
                _valid = true;
            }
        }
        else if (_valid)
            *(T*)&_data = *(T*)&other._data;
        return *this;
    }

    basic_optional& operator=(basic_optional&& other)
    {
        if (_valid != other._valid)
        {
            if (_valid)
            {
                ((T*)&_data)->~T();
                _valid = false;
            }
            else
            {
                ::new (&_data) T(std::move(*(T*)&other._data));
                _valid = true;
                ((T*)&other._data)->~T();
                other._valid = false;
            }
        }
        else if (_valid)
        {
            *(T*)&_data = std::move(*(T*)&other._data);
            ((T*)&other._data)->~T();
            other._valid = false;
        }
        return *this;
    }

    basic_optional& operator=(const T& other)
    {
        if (_valid)
            *(T*)&_data = other;
        else
        {
            ::new (&_data) T(other);
            _valid = true;
        }
        return *this;
    }

    basic_optional& operator=(T&& other)
    {
        if (_valid)
            *(T*)&_data = std::move(other);
        else
        {
            ::new (&_data) T(std::move(other));
            _valid = true;
        }
        return *this;
    }

    operator bool() const { return _valid; }

    T& operator*() { return *(T*)&_data; }
    const T& operator*() const { return *(T*)&_data; }
    T* operator->() { return (T*)&_data; }
    const T* operator->() const { return (T*)&_data; }

private:
    typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type _data;
    bool _valid;
};
*/

template <typename T>
class optional
{
public:
    optional()
        : _data2(nullptr)
    {
    }
    optional(const optional& other)
        : _data2( other._data2 ? ::new T(*other._data2) : nullptr)
    {
    }

    optional(optional&& other)
    {
        _data2 = other._data2;
        other._data2 = nullptr;
    }

    optional(const T& other)
        : _data2(::new T(other))
    {
    }

    optional(T&& other)
        : _data2(::new T(std::move(other)))
    {
    }

    ~optional()
    {
        ::delete _data2;
    }

    optional& operator=(const optional& other)
    {
        if (other._data2)
        {
            if (_data2)
                *_data2 = *other._data2;
            else
                _data2 = ::new T(*other._data2);
        }
        else
        {
            ::delete _data2;
            _data2 = nullptr;
        }
        return *this;
    }

    optional& operator=(optional&& other)
    {
        if (_data2)
            ::delete _data2;
        _data2 = other._data2;
        other._data2 = nullptr;
        return *this;
    }

    optional& operator=(const T& other)
    {
        if (_data2)
            *_data2 = other;
        else
            _data2 = ::new T(other);
        return *this;
    }

    optional& operator=(T&& other)
    {
        if (_data2)
            *_data2 = std::move(other);
        else
            _data2 = ::new T(std::move(other));
        return *this;
    }

    operator bool() const { return _data2 != nullptr; }

    T& operator*() { return *_data2; }
    const T& operator*() const { return *_data2; }
    T* operator->() { return _data2; }
    const T* operator->() const { return _data2; }

private:
    T* _data2;
};

/*
template <> class optional<int8_t> : public basic_optional<int8_t> {};
template <> class optional<uint8_t> : public basic_optional<uint8_t> {};
template <> class optional<int16_t> : public basic_optional<int16_t> {};
template <> class optional<uint16_t> : public basic_optional<uint16_t> {};
template <> class optional<int32_t> : public basic_optional<int32_t> {};
template <> class optional<uint32_t> : public basic_optional<uint32_t> {};
template <> class optional<int64_t> : public basic_optional<int64_t> {};
template <> class optional<uint64_t> : public basic_optional<uint64_t> {};
template <> class optional<float> : public basic_optional<float> {};
template <> class optional<double> : public basic_optional<double> {};
template <> class optional<bool> : public basic_optional<bool> {};

*/

struct any_attribute
{
    std::string name;
    xsd::string value;
};

}

#endif
