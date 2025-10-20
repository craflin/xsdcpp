
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
        : _data(nullptr)
    {
    }
    optional(const optional& other)
        : _data(other._data ? new T(*other._data) : nullptr)
    {
    }

    optional(optional&& other)
    {
        _data = other._data;
        other._data = nullptr;
    }

    optional(const T& other)
        : _data(new T(other))
    {
    }

    optional(T&& other)
        : _data(new T(std::move(other)))
    {
    }

    ~optional()
    {
        delete _data;
    }

    optional& operator=(const optional& other)
    {
        if (other._data)
        {
            if (_data)
                *_data = *other._data;
            else
                _data = new T(*other._data);
        }
        else
        {
            delete _data;
            _data = nullptr;
        }
        return *this;
    }

    optional& operator=(optional&& other)
    {
        if (_data)
            delete _data;
        _data = other._data;
        other._data = nullptr;
        return *this;
    }

    optional& operator=(const T& other)
    {
        if (_data)
            *_data = other;
        else
            _data = new T(other);
        return *this;
    }

    optional& operator=(T&& other)
    {
        if (_data)
            *_data = std::move(other);
        else
            _data = new T(std::move(other));
        return *this;
    }

    operator bool() const { return _data != nullptr; }

    T& operator*() { return *_data; }
    const T& operator*() const { return *_data; }
    T* operator->() { return _data; }
    const T* operator->() const { return _data; }

    friend bool operator==(const optional& lh, const T& rh) { return lh._data && *lh._data == rh; }
    friend bool operator!=(const optional& lh, const T& rh) { return !lh._data || *lh._data != rh; }
    friend bool operator==(const T& lh, const optional& rh) { return rh._data && lh == rh._data; }
    friend bool operator!=(const T& lh, const optional& rh) { return !rh._data || lh != rh._data; }
    friend bool operator==(const optional& lh, const optional& rh) { return lh._data == rh._data || (lh._data && rh._data && *lh._data == *rh._data); }
    friend bool operator!=(const optional& lh, const optional& rh) { return lh._data != rh._data && (!lh._data || !rh._data || *lh._data != *rh._data); }

private:
    T* _data;
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
