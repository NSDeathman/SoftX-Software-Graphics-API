/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Vertex.h"
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class VertexBuffer
{
    public:
    void Reserve(size_t count)
    {
        data.reserve(count);
    }
    void Add(const Vertex& v)
    {
        data.push_back(v);
    }
    void Clear()
    {
        data.clear();
    }

    size_t Size() const
    {
        return data.size();
    }
    bool IsEmpty() const
    {
        return data.empty();
    }
    const Vertex& operator[](size_t i) const
    {
        return data[i];
    }

    private:
    std::vector<Vertex> data;
};

class IndexBuffer
{
    public:
    void Reserve(size_t count)
    {
        data.reserve(count);
    }
    void Add(uint index)
    {
        data.push_back(index);
    }
    void Clear()
    {
        data.clear();
    }

    size_t Size() const
    {
        return data.size();
    }
    bool IsEmpty() const
    {
        return data.empty();
    }
    uint operator[](size_t i) const
    {
        return data[i];
    }

    private:
    std::vector<uint> data;
};

class ConstantBuffer
{
    public:
    ConstantBuffer() = default;

    template <typename T> explicit ConstantBuffer(const T& value)
    {
        static_assert(sizeof(T) <= sizeof(storage), "ConstantBuffer too large");
        std::memcpy(storage, &value, sizeof(T));
        sizeBytes = static_cast<uint>(sizeof(T));
    }

    template <typename T> const T& As() const
    {
        static_assert(sizeof(T) <= sizeof(storage), "ConstantBuffer too large");
        return *reinterpret_cast<const T*>(storage);
    }

    uint Size() const
    {
        return sizeBytes;
    }
    bool IsEmpty() const
    {
        return sizeBytes == 0;
    }

    private:
    alignas(16) uint8_t storage[256] = {};
    uint sizeBytes = 0;
};

using InstanceData = ConstantBuffer;

class InstanceBuffer
{
    public:
    void Reserve(size_t count)
    {
        data.reserve(count);
    }
    void Add(const InstanceData& instance)
    {
        data.push_back(instance);
    }
    void Clear()
    {
        data.clear();
    }

    size_t Size() const
    {
        return data.size();
    }
    bool IsEmpty() const
    {
        return data.empty();
    }
    const InstanceData& operator[](size_t i) const
    {
        return data[i];
    }

    private:
    std::vector<InstanceData> data;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
