/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

DepthBuffer::DepthBuffer(uint2 size) : resolution(size)
                                       ,widthPadded((size.x + 3u) & ~3u)
                                       ,blocks((static_cast<size_t>(widthPadded)* size.y) / 4u)
{
    if (size.x == 0 || size.y == 0)
        SOFTX_THROW(InvalidArgument("DepthBuffer: zero-sized depth buffer"));

    Clear(1.0f);
}

void DepthBuffer::Clear(float depth)
{
    __m128 depth4 = _mm_set1_ps(depth);
    __m128* ptr = blocks.data();
    size_t  count = blocks.size();

    for (size_t i = 0; i < count; ++i)
        _mm_stream_ps(reinterpret_cast<float*>(ptr + i), depth4);
    _mm_sfence();
}

float DepthBuffer::Read(int2 coords) const
{
    return FloatPtr()[coords.y * widthPadded + coords.x];
}

void DepthBuffer::Write(int2 coords, float depth)
{
    FloatPtr()[coords.y * widthPadded + coords.x] = depth;
}

float& DepthBuffer::At(int2 coords)
{
    return FloatPtr()[coords.y * widthPadded + coords.x];
}

const float& DepthBuffer::At(int2 coords) const
{
    return FloatPtr()[coords.y * widthPadded + coords.x];
}

__m128 DepthBuffer::Read4(uint2 coords) const
{
    assert(coords.x % 4 == 0);
    assert(coords.x + 3u < widthPadded && coords.y < resolution.y);
    uint blockIdx = (coords.y * widthPadded + coords.x) / 4u;
    return blocks[blockIdx];
}

void DepthBuffer::Write4(uint2 coords, __m128 depths, __m128 mask)
{
    assert(coords.x % 4 == 0);
    assert(coords.x + 3u < widthPadded && coords.y < resolution.y);
    uint blockIdx = (coords.y * widthPadded + coords.x) / 4u;
    __m128& block = blocks[blockIdx];
    block = _mm_or_ps(_mm_and_ps(depths, mask), _mm_andnot_ps(mask, block));
}

SOFTX_END
/////////////////////////////////////////////////////////////////
