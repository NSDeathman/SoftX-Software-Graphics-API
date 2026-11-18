/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Texture.h"
#include "DepthBuffer.h"
#include "Buffers.h"
#include "ResourceHandles.h"
#include "ResourcePool.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

enum class PresentationMode { Headless, Windowed };

struct DeviceConfig
{
    uint2 backBufferSize{ 800, 600 };
    PresentationMode mode = PresentationMode::Headless;
    void* hwnd = nullptr;
    uint workerThreads = 0;
};

class SOFTX_API Device
{
public:
    explicit Device(const DeviceConfig& config);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    TextureHandle GetBackBufferHandle(uint index = 0);

    VertexBufferHandle CreateVertexBuffer();
    IndexBufferHandle CreateIndexBuffer();
    TextureHandle CreateTexture(uint2 size);
    DepthBufferHandle CreateDepthBuffer(uint2 size);
    InstanceBufferHandle CreateInstanceBuffer();

    VertexBuffer* GetVertexBuffer(VertexBufferHandle h);
    IndexBuffer* GetIndexBuffer(IndexBufferHandle h);
    Texture* GetTexture(TextureHandle h);
    DepthBuffer* GetDepthBuffer(DepthBufferHandle h);
    InstanceBuffer* GetInstanceBuffer(InstanceBufferHandle h);

    void Release(VertexBufferHandle h);
    void Release(IndexBufferHandle h);
    void Release(TextureHandle h);
    void Release(DepthBufferHandle h);
    void Release(InstanceBufferHandle h);

    void Resize(uint2 newSize);
    void Present();
    void WaitIdle();

    uint64_t NextSubmissionEpoch();
    void ReportCompletion(uint64_t epoch);
    void CollectGarbage();

private:
    static constexpr size_t SOFTX_MAX_PENDING_RELEASES = 64;
    void QueueRelease(std::function<void()> deleter);

    DeviceConfig config;

    ResourcePool<VertexBuffer, ResourceType::VertexBuffer> vertexBuffers;
    ResourcePool<IndexBuffer, ResourceType::IndexBuffer> indexBuffers;
    ResourcePool<Texture, ResourceType::Texture> textures;
    ResourcePool<DepthBuffer, ResourceType::DepthBuffer> depthBuffers;
    ResourcePool<InstanceBuffer, ResourceType::InstanceBuffer> instanceBuffers;

    ResourcePool<Texture, ResourceType::Texture> backBufferPool;
    std::vector<TextureHandle> backBufferHandles;

    struct PendingRelease
    {
        uint64_t epoch;
        std::function<void()> deleter;
    };
    std::vector<PendingRelease> pendingReleases;

    std::atomic<uint64_t> submittedEpoch{ 0 };
    std::atomic<uint64_t> completedEpoch{ 0 };

#ifdef _WIN32
    void* windowDC = nullptr;
    void* dibSection = nullptr;
    void PresentWindowed();
#endif
};

SOFTX_END
/////////////////////////////////////////////////////////////////
