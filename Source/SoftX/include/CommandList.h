/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Pipeline.h"
#include "Texture.h"
#include "DepthBuffer.h"
#include "Buffers.h"
#include "ResourceHandles.h"
#include "CommandTypes.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Device;
class CommandAllocator;
class CommandQueue;

class SOFTX_API CommandList
{
public:
    explicit CommandList(Device& device);
    ~CommandList() = default;

    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    void Reset(CommandAllocator& allocator);

    void Close();

    bool IsRecording() const { return recording; }
    bool IsClosed()    const { return closed; }

    void SetPipeline(const PipelineState& pso);
    void SetRenderTarget(TextureHandle rt);
    void SetDepthBuffer(DepthBufferHandle db);
    void SetVertexBuffer(VertexBufferHandle vb);
    void SetIndexBuffer(IndexBufferHandle ib);
    void SetConstantBuffer(ConstantBuffer cb);
    void SetInstanceBuffer(InstanceBufferHandle ib);

    void Clear(float4 color, float depth = 1.0f);
    void Draw();
    void DrawIndexed();
    void DrawInstanced();
    void DrawIndexedInstanced();

private:
    friend class CommandQueue;

    void ExecuteOnDevice();

    void ExecuteClear(const ClearCommand& c);
    void ExecuteDraw(const DrawCall& d);

    Device* device = nullptr;
    CommandAllocator* allocator = nullptr;

    bool recording = false;
    bool closed = true;

    PipelineState currentPso;
    TextureHandle currentRt;
    DepthBufferHandle currentDb;
    VertexBufferHandle currentVb;
    IndexBufferHandle currentIb;
    ConstantBuffer currentCb;
    InstanceBufferHandle currentInstanceBuffer;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
