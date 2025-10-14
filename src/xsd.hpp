
#pragma once

#ifndef XSDCPP_H
#define XSDCPP_H

#include <cstdint>
#include <string>
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
        : _data2(nullptr)
    {
    }
    optional(const optional& other)
        : _data2( other._data2 ? new T(*other._data2) : nullptr)
    {
    }

    optional(optional&& other)
    {
        _data2 = other._data2;
        other._data2 = nullptr;
    }

    optional(const T& other)
        : _data2(new T(other))
    {
    }

    optional(T&& other)
        : _data2(new T(std::move(other)))
    {
    }

    ~optional()
    {
        delete _data2;
    }

    optional& operator=(const optional& other)
    {
        if (other._data2)
        {
            if (_data2)
                *_data2 = *other._data2;
            else
                _data2 = new T(*other._data2);
        }
        else
        {
            delete _data2;
            _data2 = nullptr;
        }
        return *this;
    }

    optional& operator=(optional&& other)
    {
        if (_data2)
            delete _data2;
        _data2 = other._data2;
        other._data2 = nullptr;
        return *this;
    }

    optional& operator=(const T& other)
    {
        if (_data2)
            *_data2 = other;
        else
            _data2 = new T(other);
        return *this;
    }

    optional& operator=(T&& other)
    {
        if (_data2)
            *_data2 = std::move(other);
        else
            _data2 = new T(std::move(other));
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

template <typename T>
class base
{
public:
    base()
        : _value()
    {
    }
    base(T value) 
        : _value(value)
    {
    }

    operator T() const { return _value; }
    operator T&() { return _value; }

    base& operator=(T value)
    {
        _value = value;
        return *this;
    }

private:
    T _value;
};

struct any_attribute
{
    std::string name;
    xsd::string value;
};

}

#endif
