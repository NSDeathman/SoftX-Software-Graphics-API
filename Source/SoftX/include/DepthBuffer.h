/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include <algorithm>
#include <cassert>
#include <limits>
#include <vector>
#include <intrin.h>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API DepthBuffer
{
public:
    explicit DepthBuffer(uint2 size);

    void Clear(float depth);

    float Read(int2 coords) const;
    void Write(int2 coords, float depth);
    float& At(int2 coords);
    const float& At(int2 coords) const;

    __m128 Read4(uint2 coords) const;
    void Write4(uint2 coords, __m128 depths, __m128 mask);

    uint Width() const { return resolution.x; }
    uint Height() const { return resolution.y; }
    uint WidthPadded() const { return widthPadded; }
    uint2 Size() const { return resolution; }

    float* Data() { return FloatPtr(); }
    const float* Data() const { return FloatPtr(); }

private:
    float* FloatPtr() { return reinterpret_cast<float*>(blocks.data()); }
    const float* FloatPtr() const { return reinterpret_cast<const float*>(blocks.data()); }

    uint2 resolution;
    uint widthPadded;
    std::vector<__m128> blocks;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
