/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Pipeline.h"
#include "ResourceHandles.h"
#include "Buffers.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

struct ClearCommand
{
    TextureHandle rt;
    DepthBufferHandle db;
    float4 color;
    float depth = 1.0f;
};

struct DrawCall
{
    PipelineState pso;
    TextureHandle rt;
    DepthBufferHandle db;
    VertexBufferHandle vb;
    IndexBufferHandle ib;
    ConstantBuffer cb;
    InstanceBufferHandle instanceBuffer;
    bool indexed = false;
    bool instanced = false;
};

struct Command
{
    enum class Kind { Clear, Draw } kind;
    ClearCommand clear;
    DrawCall draw;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
