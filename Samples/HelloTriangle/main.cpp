/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include <SoftX.h>
#include <AfterMath.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/////////////////////////////////////////////////////////////////
using namespace SoftX;
using namespace AfterMath;
/////////////////////////////////////////////////////////////////
Device* g_device = nullptr;
/////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hWnd);
        return 0;

    case WM_SIZE:
    {
        if (g_device && wParam != SIZE_MINIMIZED)
        {
            int newWidth = LOWORD(lParam);
            int newHeight = HIWORD(lParam);

            if (newWidth > 0 && newHeight > 0)
                g_device->Resize(uint2(newWidth, newHeight));
        }
        return 0;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    uint2 windowSize = uint2(512, 512);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "Hello Triangle";
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, (LONG)windowSize.x, (LONG)windowSize.y };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindowEx(0, 
                              "Hello Triangle", 
                              "SoftX 2 Triangle Demo",
                              WS_OVERLAPPEDWINDOW, 
                              CW_USEDEFAULT, 
                              CW_USEDEFAULT,
                              rc.right - rc.left, 
                              rc.bottom - rc.top,
                              nullptr, 
                              nullptr, 
                              hInstance, 
                              nullptr);
    if (!wnd) return -1;

    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);

    DeviceConfig config;
    config.backBufferSize = windowSize;
    config.mode = PresentationMode::Windowed;
    config.hwnd = wnd;

    Device device(config);
    g_device = &device;

    auto depthBuffer = device.CreateDepthBuffer(windowSize);

    VertexBuffer vb;
    vb.Reserve(3);
    vb.Add({ float3(-1.0f, -1.0f, 0.0f), float4(1.0f, 0.0f, 0.0f, 0.0f) });
    vb.Add({ float3(0.0f,  1.0f, 0.0f), float4(0.0f, 1.0f, 0.0f, 0.0f) });
    vb.Add({ float3(1.0f, -1.0f, 0.0f), float4(0.0f, 0.0f, 1.0f, 0.0f) });

    PipelineState pso;
    pso.viewport = Viewport(0.0f, 0.0f, (float)windowSize.x, (float)windowSize.y, 0.0f, 1.0f);
    pso.rasterizer.cullMode = CullMode::None;
    pso.depth.func = ComparisonFunc::Less;
    pso.depth.writeEnable = true;
    pso.tileSize = 64;

    pso.vertexShader = [](const Vertex& in, const ConstantBuffer&) -> Interpolant
    {
        Interpolant out;
        out.ClipSpacePosition = float4(in.Position.xyz(), 1.0f);
        out.Attributes[0] = in.Attributes[0];
        return out;
    };
    pso.pixelShader = [](const Interpolant& in, const ConstantBuffer&) -> float4
    {
        return in.Attributes[0];
    };

    CommandList cmd(device);
    uint2 lastSize = windowSize;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        uint2 currentSize = device.GetBackBuffer()->Size();
        if (currentSize.x != lastSize.x || currentSize.y != lastSize.y)
        {
            depthBuffer = device.CreateDepthBuffer(currentSize);
            pso.viewport = Viewport(0.0f, 0.0f, (float)currentSize.x, (float)currentSize.y, 0.0f, 1.0f);
            lastSize = currentSize;
        }

        cmd.Begin();
        cmd.SetPipeline(pso);
        cmd.SetRenderTarget(device.GetBackBuffer());
        cmd.SetDepthBuffer(depthBuffer.get());
        cmd.SetVertexBuffer(vb);
        cmd.Clear(float4(0.10f, 0.05f, 0.12f, 1.0f), 1.0f);
        cmd.Draw();
        cmd.End();
        cmd.Execute();

        device.Present();
    }

    g_device = nullptr;
    return 0;
}
/////////////////////////////////////////////////////////////////
