/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

static constexpr int SOFT_MAX_ATTRIBUTES_COUNT = 8;

struct Vertex
{
    float3 Position;
    float4 Attributes[SOFT_MAX_ATTRIBUTES_COUNT];

    Vertex() = default;
    Vertex(const float3& position, const float4& attribute0 = float4(0.0f, 0.0f, 0.0f, 0.0f)) : Position(position)
    {
        Attributes[0] = attribute0;
    }
};

struct Interpolant
{
    float4 ClipSpacePosition;
    float4 Attributes[SOFT_MAX_ATTRIBUTES_COUNT];

    Interpolant() = default;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
