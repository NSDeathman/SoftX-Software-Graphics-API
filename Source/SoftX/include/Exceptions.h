/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include <stdexcept>
#include <string>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Exception : public std::runtime_error
{
    public:
    explicit Exception(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class InvalidState : public Exception
{
    public:
    explicit InvalidState(const std::string& message)
        : Exception("Invalid state: " + message)
    {
    }
};

class InvalidArgument : public Exception
{
    public:
    explicit InvalidArgument(const std::string& message)
        : Exception("Invalid argument: " + message)
    {
    }
};

class PlatformError : public Exception
{
    public:
    explicit PlatformError(const std::string& message)
        : Exception("Platform error: " + message)
    {
    }
};

class NotImplemented : public Exception
{
    public:
    explicit NotImplemented(const std::string& feature)
        : Exception("Not implemented: " + feature)
    {
    }
};

class IOError : public Exception
{
    public:
    explicit IOError(const std::string& message)
        : Exception("I/O error: " + message)
    {
    }
};

class OutOfMemory : public Exception
{
    public:
    explicit OutOfMemory(const std::string& message)
        : Exception("Out of memory: " + message)
    {
    }
};

SOFTX_END
/////////////////////////////////////////////////////////////////
#ifdef NDEBUG
#define SOFTX_THROW(exc) throw exc
#else
#define SOFTX_THROW(exc) \
    do \
    { \
        std::cerr << "SOFTX_THROW: " << (exc).what() << std::endl; \
        std::abort(); \
    } while (false)
#endif
/////////////////////////////////////////////////////////////////
