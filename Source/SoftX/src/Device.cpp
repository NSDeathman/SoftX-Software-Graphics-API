/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadUtils.h"
#include <algorithm>
#ifdef _WIN32
#include <Windows.h>
#endif
/////////////////////////////////////////////////////////////////
TODO("Reimplement ASCII present to console buffer")
TODO("Implement fat thread especialy for present")
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Device::Device(const DeviceConfig& cfg) : config(cfg)
{
    if (cfg.swapChainBufferCount < 2)
        SOFTX_THROW(InvalidArgument("Device: swapChainBufferCount must be >= 2"));
    swapChainBufferCount = cfg.swapChainBufferCount;

    ThreadPoolManager::Initialize(cfg.workerThreads);
    defaultQueue = std::make_unique<CommandQueue>(*this);

    RecreateSwapChainBuffers(cfg.backBufferSize);
}

Device::~Device()
{
    WaitForPresents();

    if (defaultQueue)
        defaultQueue->WaitIdle();

#ifndef NDEBUG
    if (vertexBuffers.AliveCount() || indexBuffers.AliveCount() || textures.AliveCount() || depthBuffers.AliveCount() ||
        instanceBuffers.AliveCount())
    {
        std::cerr << "[SoftX] Device destroyed with live user resources: VB=" << vertexBuffers.AliveCount()
                  << " IB=" << indexBuffers.AliveCount() 
                  << " Tex=" << textures.AliveCount()
                  << " DB=" << depthBuffers.AliveCount() 
                  << " Inst=" << instanceBuffers.AliveCount() << std::endl;
    }
#endif

    defaultQueue.reset();
    ThreadPoolManager::Shutdown();
}

void Device::RecreateSwapChainBuffers(uint2 size)
{
    PROFILE_SCOPE("Device::RecreateSwapChainBuffers")
    WaitForPresents();

    for (auto h : swapChainBuffers)
        backBufferPool.Remove(h, true);

    swapChainBuffers.clear();
    presentFutures.clear();
    backBufferPresentFenceValues.assign(swapChainBufferCount, 0);

    for (uint32_t i = 0; i < swapChainBufferCount; ++i)
    {
        auto bb = std::make_unique<Texture>(size);
        swapChainBuffers.push_back(backBufferPool.Add(std::move(bb), true));
    }
    currentBufferIndex = 0;
}

CommandQueue& Device::GetCommandQueue()
{
    return *defaultQueue;
}
const CommandQueue& Device::GetCommandQueue() const
{
    return *defaultQueue;
}

std::unique_ptr<CommandList> Device::CreateCommandList()
{
    return std::make_unique<CommandList>(*this);
}

void Device::QueueRelease(std::function<void()> deleter)
{
    PROFILE_SCOPE("Device::QueueRelease")
    uint64_t epoch = defaultQueue ? defaultQueue->GetLastSubmissionValue() : 0;
    pendingReleases.push_back({epoch, std::move(deleter)});
    CollectGarbage();
    if (pendingReleases.size() > SOFTX_MAX_PENDING_RELEASES)
        WaitIdle();
}

VertexBufferHandle Device::CreateVertexBuffer()
{
    return vertexBuffers.Add(std::make_unique<VertexBuffer>());
}

IndexBufferHandle Device::CreateIndexBuffer()
{
    return indexBuffers.Add(std::make_unique<IndexBuffer>());
}

TextureHandle Device::CreateTexture(uint2 size)
{
    return textures.Add(std::make_unique<Texture>(size));
}

DepthBufferHandle Device::CreateDepthBuffer(uint2 size)
{
    return depthBuffers.Add(std::make_unique<DepthBuffer>(size));
}

InstanceBufferHandle Device::CreateInstanceBuffer()
{
    return instanceBuffers.Add(std::make_unique<InstanceBuffer>());
}

VertexBuffer* Device::GetVertexBuffer(VertexBufferHandle h)
{
    return vertexBuffers.Get(h);
}
IndexBuffer* Device::GetIndexBuffer(IndexBufferHandle h)
{
    return indexBuffers.Get(h);
}
DepthBuffer* Device::GetDepthBuffer(DepthBufferHandle h)
{
    return depthBuffers.Get(h);
}
InstanceBuffer* Device::GetInstanceBuffer(InstanceBufferHandle h)
{
    return instanceBuffers.Get(h);
}

Texture* Device::GetTexture(TextureHandle h)
{
    for (auto bb : swapChainBuffers)
        if (bb == h)
            return backBufferPool.Get(h);
    return textures.Get(h);
}

TextureHandle Device::GetBackBufferHandle(uint index)
{
    if (index >= swapChainBuffers.size())
        SOFTX_THROW(InvalidArgument("GetBackBufferHandle: index out of range"));
    return swapChainBuffers[index];
}

void Device::Release(VertexBufferHandle h)
{
    QueueRelease(
        [this, h]()
        {
            vertexBuffers.Remove(h);
        });
}

void Device::Release(IndexBufferHandle h)
{
    QueueRelease(
        [this, h]()
        {
            indexBuffers.Remove(h);
        });
}

void Device::Release(TextureHandle h)
{
    for (auto bb : swapChainBuffers)
        if (bb == h)
            return;
    QueueRelease(
        [this, h]()
        {
            textures.Remove(h);
        });
}

void Device::Release(DepthBufferHandle h)
{
    QueueRelease(
        [this, h]()
        {
            depthBuffers.Remove(h);
        });
}

void Device::Release(InstanceBufferHandle h)
{
    QueueRelease(
        [this, h]()
        {
            instanceBuffers.Remove(h);
        });
}

void Device::CollectGarbage()
{
    PROFILE_SCOPE("Device::CollectGarbage")
    if (!defaultQueue)
        return;
    const uint64_t safe = defaultQueue->GetCompletedValue();
    auto it = std::remove_if(pendingReleases.begin(),
                             pendingReleases.end(),
                             [safe](const PendingRelease& p)
                             {
                                 if (p.epoch <= safe)
                                 {
                                     p.deleter();
                                     return true;
                                 }
                                 return false;
                             });
    pendingReleases.erase(it, pendingReleases.end());
}

void Device::WaitIdle()
{
    PROFILE_SCOPE("Device::WaitIdle")
    if (defaultQueue)
        defaultQueue->WaitIdle();
    CollectGarbage();
}

void Device::Resize(uint2 newSize)
{
    PROFILE_SCOPE("Device::Resize")
    if (newSize.x == config.backBufferSize.x && newSize.y == config.backBufferSize.y)
        return;

    WaitForPresents();

    if (defaultQueue)
        defaultQueue->WaitIdle();

    config.backBufferSize = newSize;
    RecreateSwapChainBuffers(newSize);

    CollectGarbage();
}

uint64_t Device::BeginFrame()
{
    PROFILE_SCOPE("Device::BeginFrame")
    const uint32_t slot = currentBufferIndex;
    return backBufferPresentFenceValues[slot];
}

void Device::Present(uint64_t renderFenceValue)
{
    PROFILE_SCOPE("Device::Present")
    const uint32_t slot = currentBufferIndex;

    const uint64_t presentValue = ++nextPresentFenceValue;
    backBufferPresentFenceValues[slot] = presentValue;

#ifdef _WIN32
    if (config.mode == PresentationMode::Windowed)
    {
        ThreadPoolManager::Get().enqueueBackground(
            [this, renderFenceValue, presentValue, slot]()
            {
                PROFILE_SCOPE("Device::Present::Blit")
                try
                {
                    {                    
                        PROFILE_SCOPE("Device::Present::WaitForFenceValue")
                        defaultQueue->WaitForFenceValue(renderFenceValue);
                    }
                    BlitBackBufferToWindow(backBufferPool.Get(swapChainBuffers[slot]));
                }
                catch (...) {  }
                presentFence.Signal(presentValue);
            });
    }
    else
#endif
    {
        defaultQueue->SignalWithValue(presentFence, presentValue);
    }

    currentBufferIndex = (currentBufferIndex + 1) % swapChainBufferCount;
}

void Device::WaitForPresent(uint64_t presentFenceValue)
{
    PROFILE_SCOPE("Device::WaitForPresent")
    if (presentFenceValue != 0)
        presentFence.Wait(presentFenceValue);
}

void Device::WaitForPresents()
{
    PROFILE_SCOPE("Device::WaitForPresents")
    const uint64_t target = nextPresentFenceValue.load();
    if (target != 0)
        presentFence.Wait(target);
}

#ifdef _WIN32
void Device::BlitBackBufferToWindow(Texture* bb)
{
    PROFILE_SCOPE("Device::BlitBackBufferToWindow")
    if (!bb) return;
    HWND hwnd = static_cast<HWND>(config.hwnd);
    if (!hwnd) return;

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    const uint w = bb->Width();
    const uint h = bb->Height();
    if (w == 0 || h == 0) { ReleaseDC(hwnd, hdc); return; }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(w);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(h);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    const size_t totalBytes = static_cast<size_t>(w) * h * 4;
    if (blitScratch.size() < totalBytes)
        blitScratch.resize(totalBytes);

    uint8_t* dst = blitScratch.data();
    const __m128* src = bb->GetRawPixels();
    const uint srcStride = bb->PaddedWidth();
    const uint dstStride = w * 4;

    {
        PROFILE_SCOPE("Blit::PackBGRA")

        const __m128 scale = _mm_set1_ps(255.0f);
        std::atomic<uint> nextRow{ 0 };

        auto worker = [&]()
        {
            PROFILE_THREAD("SoftX Blit Pack Worker")
            uint y;
            while ((y = nextRow.fetch_add(1, std::memory_order_relaxed)) < h)
            {
                const __m128* srcRow = src + static_cast<size_t>(y) * srcStride;
                uint8_t* dstRow = dst + static_cast<size_t>(y) * dstStride;

                for (uint x = 0; x < w; ++x)
                {
                    // float4 (R,G,B,A) -> 4 int32 -> 4 int16 (saturated) -> 4 uint8 (saturated) ->pack to uint32
                    __m128 c = _mm_mul_ps(srcRow[x], scale);
                    __m128i i32 = _mm_cvttps_epi32(c);            // truncate
                    __m128i i16 = _mm_packs_epi32(i32, i32);      // saturate int32 -> int16
                    __m128i i8 = _mm_packus_epi16(i16, i16);      // saturate int16 -> uint8

                    uint32_t rgba = static_cast<uint32_t>(_mm_cvtsi128_si32(i8));
                    // rgba = R | (G<<8) | (B<<16) | (A<<24) to BGRA
                    uint32_t bgra = 0xFF000000u
                                    | ((rgba & 0x000000FFu) << 16)      // R -> byte 2
                                    | (rgba & 0x0000FF00u)              // G -> byte 1
                                    | ((rgba & 0x00FF0000u) >> 16);     // B -> byte 0

                    *reinterpret_cast<uint32_t*>(dstRow + x * 4) = bgra;
                }
            }
        };
        ThreadUtils::DispatchWorkers(worker);
    }

    RECT client;
    GetClientRect(hwnd, &client);

    if (client.right == static_cast<LONG>(w) && client.bottom == static_cast<LONG>(h))
    {
        PROFILE_SCOPE("Blit::SetDIBits")
        SetDIBitsToDevice(hdc,
                          0, 0,
                          static_cast<int>(w), static_cast<int>(h),
                          0, 0, 0, h,
                          dst, &bmi, DIB_RGB_COLORS);
    }
    else
    {
        PROFILE_SCOPE("Blit::StretchDIBits")
        StretchDIBits(hdc,
                      0, 0, client.right, client.bottom,
                      0, 0, static_cast<int>(w), static_cast<int>(h),
                      dst, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }

    ReleaseDC(hwnd, hdc);
}
#endif

SOFTX_END
/////////////////////////////////////////////////////////////////
