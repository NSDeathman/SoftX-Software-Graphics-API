/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "ThreadUtils.h"
#ifdef _WIN32
#include <Windows.h>
#endif
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

Device::Device(const DeviceConfig& cfg) : config(cfg)
{
    ThreadPoolManager::Initialize(cfg.workerThreads);
    backBuffers.push_back(std::make_shared<Texture>(cfg.backBufferSize));
}

Device::~Device()
{
#ifdef _WIN32
    if (dibSection) DeleteObject(static_cast<HBITMAP>(dibSection));
    if (windowDC) ReleaseDC(static_cast<HWND>(config.hwnd), static_cast<HDC>(windowDC));
#endif
    ThreadPoolManager::Shutdown();
}

Texture* Device::GetBackBuffer(uint index)
{
    if (index >= backBuffers.size())
        SOFTX_THROW(InvalidState("GetBackBuffer: index out of range"));
    return backBuffers[index].get();
}

std::shared_ptr<DepthBuffer> Device::CreateDepthBuffer(uint2 size)
{
    return std::make_shared<DepthBuffer>(size);
}

void Device::Resize(uint2 newSize)
{
    if (newSize.x == config.backBufferSize.x && newSize.y == config.backBufferSize.y)
        return;

    config.backBufferSize = newSize;
    backBuffers.clear();
    backBuffers.push_back(std::make_shared<Texture>(newSize));
}

void Device::WaitIdle()
{
    ThreadPoolManager::Get().wait();
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
}

#ifdef _WIN32
void Device::PresentWindowed()
{
    HWND hwnd = static_cast<HWND>(config.hwnd);
    if (!hwnd) return;

    Texture* bb = GetBackBuffer();
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
