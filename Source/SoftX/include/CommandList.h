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
#include <future>
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
    void SetRenderTarget(Texture* rt);
    void SetDepthBuffer(DepthBuffer* db);
    void SetVertexBuffer(VertexBuffer vb);
    void SetIndexBuffer(IndexBuffer ib);
    void SetConstantBuffer(ConstantBuffer cb);
    void SetInstanceBuffer(InstanceBuffer ib);

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
        Texture* rt = nullptr;
        DepthBuffer* db = nullptr;
        VertexBuffer vb;
        IndexBuffer ib;
        ConstantBuffer cb;
        InstanceBuffer instanceBuffer;
        bool indexed = false;
        bool instanced = false;
    };

    struct ClearCommand
    {
        Texture* rt = nullptr;
        DepthBuffer* db = nullptr;
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
    Texture* currentRt = nullptr;
    DepthBuffer* currentDb = nullptr;
    VertexBuffer currentVb;
    IndexBuffer currentIb;
    ConstantBuffer currentCb;
    InstanceBuffer currentInstanceBuffer;

    std::vector<Command> commands;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
