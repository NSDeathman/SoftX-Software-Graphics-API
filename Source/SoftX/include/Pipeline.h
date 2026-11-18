/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Enums.h"
#include "Vertex.h"
#include "Buffers.h"
#include <functional>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

struct Viewport
{
    float2 pos{0.0f, 0.0f};
    float2 size{0.0f, 0.0f};
    float minZ = 0.0f;
    float maxZ = 1.0f;

    Viewport() = default;
    Viewport(float x, float y, float w, float h, float minZ_ = 0.0f, float maxZ_ = 1.0f) : pos(x, y), size(w, h), minZ(minZ_), maxZ(maxZ_)
    {
    }
};

struct RasterizerState
{
    CullMode cullMode = CullMode::Back;
};

struct DepthState
{
    ComparisonFunc func = ComparisonFunc::Less;
    bool writeEnable = true;
};

using VertexShader = std::function<Interpolant(const Vertex&, const ConstantBuffer&)>;
using PixelShader = std::function<float4(const Interpolant&, const ConstantBuffer&)>;
using InstanceShader = std::function<Interpolant(const Interpolant&, const InstanceData&)>;

struct PipelineState
{
    Viewport viewport;
    RasterizerState rasterizer;
    DepthState depth;
    uint tileSize = 64;

    VertexShader vertexShader;
    InstanceShader instanceShader;
    PixelShader pixelShader;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
