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
#include <intrin.h>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API Texture
{
public:
    explicit Texture(uint2 size);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Clear(float4 color);

    void StreamWrite(uint2 coords, __m128 color);
    void Write(uint2 coords, float4 color);
    float4 Read(uint2 coords) const;

    void SaveToTGA(const char* filename) const;

    uint Width() const { return resolution.x; }
    uint Height() const { return resolution.y; }
    uint2 Size() const { return resolution; }

private:
    uint2 resolution;
    uint widthPadded;
    __m128* pixels = nullptr;
    size_t pixelCount = 0;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
