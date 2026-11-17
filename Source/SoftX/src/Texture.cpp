/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadUtils.h"
#include <fstream>
#include <iostream>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Texture::Texture(uint2 size) : resolution(size)
                               ,widthPadded((size.x + 3u) & ~3u)
                               ,pixelCount(static_cast<size_t>(widthPadded)* size.y)
{
    if (size.x == 0 || size.y == 0)
        SOFTX_THROW(InvalidArgument("Texture: zero-sized texture"));

    pixels = new __m128[pixelCount];
}

Texture::~Texture()
{
    delete[] pixels;
    pixels = nullptr;
}

void Texture::StreamWrite(uint2 coords, __m128 color)
{
    assert(coords.x < resolution.x&& coords.y < resolution.y);
    _mm_stream_ps(reinterpret_cast<float*>(pixels + (coords.y * widthPadded + coords.x)), color);
}

void Texture::Write(uint2 coords, float4 color)
{
    assert(coords.x < resolution.x&& coords.y < resolution.y);
    pixels[coords.y * widthPadded + coords.x] = color.simd_;
}

float4 Texture::Read(uint2 coords) const
{
    assert(coords.x < resolution.x&& coords.y < resolution.y);
    float4 result;
    result.simd_ = pixels[coords.y * widthPadded + coords.x];
    return result;
}

void Texture::Clear(float4 color)
{
    size_t levelPixels = static_cast<size_t>(resolution.x) * resolution.y;
    __m128* dest = pixels;
    __m128 col = color.simd_;

    const uint32_t numThreads = ThreadPoolManager::Get().threadCount();
    if (numThreads <= 1 || levelPixels < 64 * 64)
    {
        for (size_t i = 0; i < levelPixels; ++i)
            _mm_stream_ps(reinterpret_cast<float*>(dest + i), col);
    }
    else
    {
        std::atomic<size_t> next(0);
        for (uint32_t t = 0; t < numThreads; ++t)
        {
            ThreadPoolManager::Get().enqueue([&]()
                {
                    size_t i;
                    while ((i = next.fetch_add(1024)) < levelPixels)
                    {
                        size_t end = std::min(i + 1024, levelPixels);
                        for (; i < end; ++i)
                            _mm_stream_ps(reinterpret_cast<float*>(dest + i), col);
                    }
                });
        }
        ThreadPoolManager::Get().wait();
    }
    _mm_sfence();
}

void Texture::SaveToTGA(const char* filename) const
{
    uint w = Width();
    uint h = Height();

    uint8_t header[18] = { 0 };
    header[2] = 2;
    header[12] = w & 0xFF;
    header[13] = (w >> 8) & 0xFF;
    header[14] = h & 0xFF;
    header[15] = (h >> 8) & 0xFF;
    header[16] = 32;
    header[17] = 8 | (1 << 5);

    std::ofstream file(filename, std::ios::binary);
    if (!file)
    {
        std::cerr << "Cannot open file for writing: " << filename << std::endl;
        return;
    }
    file.write(reinterpret_cast<const char*>(header), 18);

    for (uint y = 0; y < h; ++y)
    {
        for (uint x = 0; x < w; ++x)
        {
            float4 color = Read(uint2(x, y));
            float rgba[4];
            _mm_storeu_ps(rgba, color.simd_);
            uint8_t b = static_cast<uint8_t>(AfterMath::clamp(rgba[2] * 255.0f, 0.0f, 255.0f));
            uint8_t g = static_cast<uint8_t>(AfterMath::clamp(rgba[1] * 255.0f, 0.0f, 255.0f));
            uint8_t r = static_cast<uint8_t>(AfterMath::clamp(rgba[0] * 255.0f, 0.0f, 255.0f));
            uint8_t a = static_cast<uint8_t>(AfterMath::clamp(rgba[3] * 255.0f, 0.0f, 255.0f));
            uint8_t pixel[4] = { b, g, r, a };
            file.write(reinterpret_cast<const char*>(pixel), 4);
        }
    }
    file.close();
    std::cout << "Texture saved to " << filename << std::endl;
}

SOFTX_END
/////////////////////////////////////////////////////////////////
