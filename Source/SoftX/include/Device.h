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

    Texture* GetBackBuffer(uint index = 0);
    std::shared_ptr<DepthBuffer> CreateDepthBuffer(uint2 size);

    void Resize(uint2 newSize);

    void Present();
    void WaitIdle();

private:
    DeviceConfig config;
    std::vector<std::shared_ptr<Texture>> backBuffers;

#ifdef _WIN32
    void* windowDC = nullptr;
    void* dibSection = nullptr;
    void PresentWindowed();
#endif
};

SOFTX_END
/////////////////////////////////////////////////////////////////
