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
#include <chrono>
#include <cmath>
/////////////////////////////////////////////////////////////////
using namespace SoftX;
using namespace AfterMath;
/////////////////////////////////////////////////////////////////
Device* g_device = nullptr;
uint2 g_windowSize = uint2(800, 600);
float g_cameraAngle = 0.0f;
/////////////////////////////////////////////////////////////////
struct alignas(16) CbData
{
    float4x4 modelViewProjection;
    float4 objectColor;
};
/////////////////////////////////////////////////////////////////
void CreateCube(VertexBuffer& vb, IndexBuffer& ib)
{
    const float3 positions[8] = {
        {-0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},
        {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f},
    };
    const int faces[6][4] = {
        {1, 5, 6, 2}, // +X
        {4, 0, 3, 7}, // -X
        {2, 6, 7, 3}, // +Y
        {4, 5, 1, 0}, // -Y
        {5, 4, 7, 6}, // +Z
        {0, 1, 2, 3}, // -Z
    };
    vb.Reserve(24);
    ib.Reserve(36);
    for (auto& f : faces)
    {
        uint base = static_cast<uint>(vb.Size());
        for (int idx : f)
        {
            Vertex v;
            v.Position = positions[idx];
            vb.Add(v);
        }
        ib.Add(base + 0);
        ib.Add(base + 1);
        ib.Add(base + 2);
        ib.Add(base + 2);
        ib.Add(base + 3);
        ib.Add(base + 0);
    }
}
/////////////////////////////////////////////////////////////////
Interpolant CubeVS(const Vertex& input, const ConstantBuffer& cb)
{
    const CbData& data = cb.As<CbData>();
    Interpolant output;
    output.ClipSpacePosition = float4(input.Position.xyz(), 1.0f) * data.modelViewProjection;
    output.Attributes[0] = data.objectColor;
    return output;
}
float4 CubePS(const Interpolant& input, const ConstantBuffer&)
{
    return input.Attributes[0];
}
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
                int w = LOWORD(lParam);
                int h = HIWORD(lParam);
                if (w > 0 && h > 0)
                    g_device->Resize(uint2(w, h));
            }
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "SoftXCubeDemo";
    RegisterClassEx(&wc);

    RECT rc = {0, 0, (LONG)g_windowSize.x, (LONG)g_windowSize.y};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindowEx(0,
                              "SoftXCubeDemo",
                              "SoftX Cube Demo (Async Present)",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              rc.right - rc.left,
                              rc.bottom - rc.top,
                              nullptr,
                              nullptr,
                              hInstance,
                              nullptr);
    if (!wnd)
        return -1;

    ShowWindow(wnd, nCmdShow);
    UpdateWindow(wnd);

    DeviceConfig config;
    config.backBufferSize = g_windowSize;
    config.mode = PresentationMode::Windowed;
    config.hwnd = wnd;
    config.swapChainBufferCount = 3;

    Device device(config);
    g_device = &device;

    DepthBufferHandle depthBufferHandle = device.CreateDepthBuffer(g_windowSize);
    VertexBufferHandle vbHandle = device.CreateVertexBuffer();
    IndexBufferHandle ibHandle = device.CreateIndexBuffer();
    CreateCube(*device.GetVertexBuffer(vbHandle), *device.GetIndexBuffer(ibHandle));

    PipelineState pso;
    pso.viewport = Viewport(0.0f, 0.0f, (float)g_windowSize.x, (float)g_windowSize.y, 0.0f, 1.0f);
    pso.rasterizer.cullMode = CullMode::None;
    pso.depth.func = ComparisonFunc::Less;
    pso.depth.writeEnable = true;
    pso.tileSize = 64;
    pso.vertexShader = CubeVS;
    pso.pixelShader = CubePS;

    const uint32_t ringSize = device.GetSwapChainBufferCount();
    std::vector<CommandAllocator> allocators(ringSize);
    std::vector<std::unique_ptr<CommandList>> lists(ringSize);
    for (uint32_t i = 0; i < ringSize; ++i)
        lists[i] = device.CreateCommandList();

    CommandQueue& queue = device.GetCommandQueue();

    uint2 lastSize = g_windowSize;
    auto lastTime = std::chrono::steady_clock::now();

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                break;
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        g_cameraAngle += 0.6f * dt;

        Texture* backBuffer = device.GetTexture(device.GetCurrentBackBufferHandle());
        if (!backBuffer)
            continue;
        uint2 currentSize = backBuffer->Size();
        if (currentSize.x != lastSize.x || currentSize.y != lastSize.y)
        {
            g_windowSize = currentSize;

            device.Release(depthBufferHandle);
            depthBufferHandle = device.CreateDepthBuffer(currentSize);

            pso.viewport = Viewport(0.0f, 0.0f, (float)currentSize.x, (float)currentSize.y, 0.0f, 1.0f);
            lastSize = currentSize;
        }

        const uint64_t waitPresent = device.BeginFrame();

        const float radius = 3.0f;
        const float height = 1.8f;
        float3 eye = float3(cosf(g_cameraAngle) * radius, height, sinf(g_cameraAngle) * radius);
        float4x4 view = look_at(eye, float3(0.0f, 0.0f, 0.0f));
        float4x4 proj = perspective(Constants::degrees_to_radians(60.0f), float(g_windowSize.x) / float(g_windowSize.y));
        float4x4 viewProj = view * proj;

        CbData cb;
        cb.modelViewProjection = viewProj;
        cb.objectColor = float4(0.95f, 0.75f, 0.25f, 1.0f);

        device.WaitForPresent(waitPresent);

        const uint32_t slot = device.GetCurrentBackBufferIndex();

        allocators[slot].Reset();
        lists[slot]->Reset(allocators[slot]);

        lists[slot]->SetPipeline(pso);
        lists[slot]->SetRenderTarget(device.GetCurrentBackBufferHandle());
        lists[slot]->SetDepthBuffer(depthBufferHandle);
        lists[slot]->SetVertexBuffer(vbHandle);
        lists[slot]->SetIndexBuffer(ibHandle);
        lists[slot]->SetConstantBuffer(ConstantBuffer(cb));
        lists[slot]->Clear(float4(0.05f, 0.10f, 0.18f, 1.0f), 1.0f);
        lists[slot]->DrawIndexed();
        lists[slot]->Close();

        const uint64_t submitFence = queue.Execute(*lists[slot]);
        device.Present(submitFence);
    }

    device.WaitForPresents();
    device.WaitIdle();

    device.Release(depthBufferHandle);
    device.Release(vbHandle);
    device.Release(ibHandle);

    for (auto& l : lists)
        l.reset();

    g_device = nullptr;
    return int(msg.wParam);
}
/////////////////////////////////////////////////////////////////
