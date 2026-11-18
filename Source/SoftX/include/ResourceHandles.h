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

enum class ResourceType : uint8_t 
{
    VertexBuffer, 
    IndexBuffer, 
    ConstantBuffer, 
    InstanceBuffer,
    Texture, 
    DepthBuffer
};

template<ResourceType Type>
struct ResourceHandle 
{
    uint32_t index = 0xFFFFFFFFu;
    uint32_t generation = 0;
    bool IsValid() const { return index != 0xFFFFFFFFu; }
    bool operator==(const ResourceHandle& o) const 
    {
        return index == o.index && generation == o.generation;
    }
    bool operator!=(const ResourceHandle& o) const { return !(*this == o); }
};

using VertexBufferHandle = ResourceHandle<ResourceType::VertexBuffer>;
using IndexBufferHandle = ResourceHandle<ResourceType::IndexBuffer>;
using TextureHandle = ResourceHandle<ResourceType::Texture>;
using DepthBufferHandle = ResourceHandle<ResourceType::DepthBuffer>;
using InstanceBufferHandle = ResourceHandle<ResourceType::InstanceBuffer>;
using ConstantBufferHandle = ResourceHandle<ResourceType::ConstantBuffer>;

SOFTX_END
/////////////////////////////////////////////////////////////////
