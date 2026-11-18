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
SOFTX_BEGIN

Device::Device(const DeviceConfig& cfg) : config(cfg)
{
    ThreadPoolManager::Initialize(cfg.workerThreads);

    auto bb = std::make_unique<Texture>(cfg.backBufferSize);
    backBufferHandles.push_back(backBufferPool.Add(std::move(bb), /*permanent=*/true));
}

Device::~Device()
{
#ifdef _WIN32
    if (dibSection) DeleteObject(static_cast<HBITMAP>(dibSection));
    if (windowDC)   ReleaseDC(static_cast<HWND>(config.hwnd), static_cast<HDC>(windowDC));
#endif

    ThreadPoolManager::Get().wait();

#ifndef NDEBUG
    if (vertexBuffers.AliveCount() || 
        indexBuffers.AliveCount() || 
        textures.AliveCount() || 
        depthBuffers.AliveCount() || 
        instanceBuffers.AliveCount())
    {
        std::cerr << "[SoftX] Device destroyed with live user resources: VB="
                  << vertexBuffers.AliveCount()
                  << " IB=" << indexBuffers.AliveCount()
                  << " Tex=" << textures.AliveCount()
                  << " DB=" << depthBuffers.AliveCount()
                  << " Inst=" << instanceBuffers.AliveCount()
                  << std::endl;
    }
#endif

    ThreadPoolManager::Shutdown();
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

VertexBuffer* Device::GetVertexBuffer(VertexBufferHandle h) { return vertexBuffers.Get(h); }
IndexBuffer* Device::GetIndexBuffer(IndexBufferHandle  h) { return indexBuffers.Get(h); }
DepthBuffer* Device::GetDepthBuffer(DepthBufferHandle  h) { return depthBuffers.Get(h); }
InstanceBuffer* Device::GetInstanceBuffer(InstanceBufferHandle h) { return instanceBuffers.Get(h); }

Texture* Device::GetTexture(TextureHandle h)
{
    if (Texture* t = textures.Get(h)) return t;
    return backBufferPool.Get(h);
}

TextureHandle Device::GetBackBufferHandle(uint index)
{
    if (index >= backBufferHandles.size())
        SOFTX_THROW(InvalidArgument("GetBackBufferHandle: index out of range"));
    return backBufferHandles[index];
}

void Device::QueueRelease(std::function<void()> deleter)
{
    pendingReleases.push_back({ submittedEpoch.load(), std::move(deleter) });

    CollectGarbage();

    if (pendingReleases.size() > SOFTX_MAX_PENDING_RELEASES)
        WaitIdle();
}

void Device::Release(VertexBufferHandle h)
{
    QueueRelease([this, h]() { vertexBuffers.Remove(h); });
}

void Device::Release(IndexBufferHandle h)
{
    QueueRelease([this, h]() { indexBuffers.Remove(h); });
}

void Device::Release(TextureHandle h)
{
    for (auto bb : backBufferHandles)
        if (bb == h)
            return;

    QueueRelease([this, h]() { textures.Remove(h); });
}

void Device::Release(DepthBufferHandle h)
{
    QueueRelease([this, h]() { depthBuffers.Remove(h); });
}

void Device::Release(InstanceBufferHandle h)
{
    QueueRelease([this, h]() { instanceBuffers.Remove(h); });
}

uint64_t Device::NextSubmissionEpoch()
{
    return ++submittedEpoch;
}

void Device::ReportCompletion(uint64_t epoch)
{
    uint64_t prev = completedEpoch.load();
    while (prev < epoch && !completedEpoch.compare_exchange_weak(prev, epoch)) {}
}

void Device::CollectGarbage()
{
    const uint64_t safe = completedEpoch.load();
    auto it = std::remove_if(pendingReleases.begin(), pendingReleases.end(),
        [safe](const PendingRelease& p)
        {
            if (p.epoch <= safe) { p.deleter(); return true; }
            return false;
        });
    pendingReleases.erase(it, pendingReleases.end());
}

void Device::Resize(uint2 newSize)
{
    if (newSize.x == config.backBufferSize.x && newSize.y == config.backBufferSize.y)
        return;

    config.backBufferSize = newSize;

    const uint64_t epoch = submittedEpoch.load();
    for (auto h : backBufferHandles)
        pendingReleases.push_back({ epoch, [this, h]() { backBufferPool.Remove(h, /*force=*/true); } });
    backBufferHandles.clear();

    auto bb = std::make_unique<Texture>(newSize);
    backBufferHandles.push_back(backBufferPool.Add(std::move(bb), /*permanent=*/true));

    CollectGarbage();
}

void Device::WaitIdle()
{
    ThreadPoolManager::Get().wait();
    completedEpoch.store(submittedEpoch.load());
    CollectGarbage();
}

void Device::Present()
{
    switch (config.mode)
    {
    case PresentationMode::Headless:
        break;
#ifdef _WIN32
    case PresentationMode::Windowed:
        PresentWindowed();
        break;
#endif
    }
    CollectGarbage();
}

#ifdef _WIN32
void Device::PresentWindowed()
{
    HWND hwnd = static_cast<HWND>(config.hwnd);
    if (!hwnd) return;

    Texture* bb = GetTexture(GetBackBufferHandle());
    if (!bb) return;

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(bb->Width());
    bmi.bmiHeader.biHeight = -static_cast<LONG>(bb->Height());
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint8_t> bytes(bb->Width() * bb->Height() * 4);
    for (uint y = 0; y < bb->Height(); ++y)
    {
        for (uint x = 0; x < bb->Width(); ++x)
        {
            float4 c = bb->Read(uint2(x, y));
            uint8_t* px = bytes.data() + (y * bb->Width() + x) * 4;
            px[0] = static_cast<uint8_t>(AfterMath::clamp(c.z * 255.0f, 0.0f, 255.0f));
            px[1] = static_cast<uint8_t>(AfterMath::clamp(c.y * 255.0f, 0.0f, 255.0f));
            px[2] = static_cast<uint8_t>(AfterMath::clamp(c.x * 255.0f, 0.0f, 255.0f));
            px[3] = 255;
        }
    }

    RECT client; GetClientRect(hwnd, &client);
    StretchDIBits(hdc,
        0, 0, client.right, client.bottom,
        0, 0, static_cast<int>(bb->Width()), static_cast<int>(bb->Height()),
        bytes.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(hwnd, hdc);
}
#endif

SOFTX_END
/////////////////////////////////////////////////////////////////
