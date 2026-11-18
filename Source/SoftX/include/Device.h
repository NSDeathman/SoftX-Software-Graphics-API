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
#include <future>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class CommandQueue;
class CommandList;

enum class PresentationMode { Headless, Windowed };

struct DeviceConfig
{
    uint2 backBufferSize{ 800, 600 };
    PresentationMode mode = PresentationMode::Headless;
    void* hwnd = nullptr;
    uint workerThreads = 0;
    uint swapChainBufferCount = 2;
    uint framesInFlight = 0;
};

class SOFTX_API Device
{
public:
    explicit Device(const DeviceConfig& config);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    uint32_t GetSwapChainBufferCount() const { return swapChainBufferCount; }
    uint32_t GetCurrentBackBufferIndex() const { return currentBufferIndex; }
    uint32_t GetFramesInFlight() const { return config.framesInFlight ? config.framesInFlight : swapChainBufferCount; }

    TextureHandle GetBackBufferHandle(uint index);
    TextureHandle GetCurrentBackBufferHandle() const { return swapChainBuffers[currentBufferIndex]; }

    uint64_t BeginFrame();

    void WaitForPresent(uint64_t presentFenceValue);

    void Present(uint64_t renderFenceValue);
    void WaitForPresents();

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

    CommandQueue& GetCommandQueue();
    const CommandQueue& GetCommandQueue() const;
    std::unique_ptr<CommandList> CreateCommandList();

    void Resize(uint2 newSize);
    void WaitIdle();
    void CollectGarbage();

private:
    static constexpr size_t SOFTX_MAX_PENDING_RELEASES = 64;

    void QueueRelease(std::function<void()> deleter);
    void RecreateSwapChainBuffers(uint2 size);

#ifdef _WIN32
    void BlitBackBufferToWindow(Texture* bb);
    std::vector<uint8_t> blitScratch;
#endif

    DeviceConfig config;

    ResourcePool<VertexBuffer, ResourceType::VertexBuffer> vertexBuffers;
    ResourcePool<IndexBuffer, ResourceType::IndexBuffer> indexBuffers;
    ResourcePool<Texture, ResourceType::Texture> textures;
    ResourcePool<DepthBuffer, ResourceType::DepthBuffer> depthBuffers;
    ResourcePool<InstanceBuffer, ResourceType::InstanceBuffer> instanceBuffers;

    ResourcePool<Texture, ResourceType::Texture> backBufferPool;
    std::vector<TextureHandle> swapChainBuffers;
    std::vector<uint64_t> backBufferFences;
    uint32_t swapChainBufferCount = 2;
    uint32_t currentBufferIndex = 0;
    Fence presentFence;
    std::vector<uint64_t> backBufferPresentFenceValues;
    std::atomic<uint64_t> nextPresentFenceValue{ 0 };
    std::vector<std::future<void>> presentFutures;

    struct PendingRelease
    {
        uint64_t epoch;
        std::function<void()> deleter;
    };
    std::vector<PendingRelease> pendingReleases;

    std::unique_ptr<CommandQueue> defaultQueue;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
