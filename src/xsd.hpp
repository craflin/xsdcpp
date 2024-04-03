
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

template <typename T>
class optional
{
public:
    optional()
        : _valid(false)
    {
    }
    optional(const optional& other)
        : _valid(other._valid)
    {
        if (_valid)
            ::new (&_data) T(*(T*)&other._data);
    }

    optional(optional&& other)
        : _valid(other._valid)
    {
        if (_valid)
        {
            ::new (&_data) T(std::move(*(T*)&other._data));
            ((T*)&other._data)->~T();
            other._valid = false;
        }
    }

    optional(const T& other)
        : _valid(true)
    {
        ::new (&_data) T(other);
    }

    optional(T&& other)
        : _valid(true)
    {
        ::new (&_data) T(std::move(other));
    }

    ~optional()
    {
        if (_valid)
            ((T*)&_data)->~T();
    }

    optional& operator=(const optional& other)
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

    optional& operator=(optional&& other)
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

    optional& operator=(const T& other)
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

    optional& operator=(T&& other)
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

}

#endif
