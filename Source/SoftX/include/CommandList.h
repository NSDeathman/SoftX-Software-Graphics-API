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
#include <future>
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Device;

class SOFTX_API CommandList
{
public:
    explicit CommandList(Device& device);
    ~CommandList();

    CommandList(const CommandList&) = delete;
    CommandList& operator=(const CommandList&) = delete;

    void Begin();
    void End();
    void Reset();

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

    void Execute();
    std::future<void> ExecuteAsync();

private:
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

    struct ClearCommand
    {
        TextureHandle rt;
        DepthBufferHandle db;
        float4 color;
        float depth = 1.0f;
    };

    struct Command
    {
        enum class Kind { Clear, Draw } kind;
        ClearCommand clear;
        DrawCall draw;
    };

    void ExecuteClear(const ClearCommand& c);
    void ExecuteDraw(const DrawCall& d);

    Device* device;
    bool recording = false;

    PipelineState currentPso;
    TextureHandle currentRt;
    DepthBufferHandle currentDb;
    VertexBufferHandle currentVb;
    IndexBufferHandle currentIb;
    ConstantBuffer currentCb;
    InstanceBufferHandle currentInstanceBuffer;

    std::vector<Command> commands;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
