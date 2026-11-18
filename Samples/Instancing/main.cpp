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
#include <vector>
#include <memory>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <random>
/////////////////////////////////////////////////////////////////
#ifdef USE_OPTICK
#pragma message("USE_OPTICK is defined")
#else
#pragma message("USE_OPTICK is NOT defined")
#endif

#ifdef ENABLE_PROFILER
#pragma message("ENABLE_PROFILER is defined")
#else
#pragma message("ENABLE_PROFILER is NOT defined")
#endif
/////////////////////////////////////////////////////////////////
using namespace SoftX;
using namespace AfterMath;
/////////////////////////////////////////////////////////////////
static constexpr int CUBE_COUNT = 1000;
static constexpr float CAMERA_RADIUS = 30.0f;
static constexpr float CAMERA_HEIGHT = 20.0f;
static constexpr float CAMERA_SPEED = 0.4f;
static constexpr uint TILE_SIZE = 64;
/////////////////////////////////////////////////////////////////
Device* g_device = nullptr;
uint2 g_windowSize = uint2(1280, 768);
bool g_useInstancing = true;
float g_fps = 0.0f;
int g_frameCount = 0;
float g_cameraAngle = 0.0f;
std::chrono::steady_clock::time_point g_fpsLastTime;
/////////////////////////////////////////////////////////////////
struct alignas(16) CbData
{
    float4x4 modelViewProjection;
    float4 objectColor;
};

struct alignas(16) PerInstanceData
{
    float4x4 worldMatrix;
    float4 color;
};
/////////////////////////////////////////////////////////////////
Interpolant SharedVS(const Vertex& input, const ConstantBuffer&)
{
    Interpolant output;
    output.ClipSpacePosition = float4(input.Position.xyz(), 1.0f);
    return output;
}

Interpolant InstanceTransform(const Interpolant& input, const InstanceData& inst)
{
    const PerInstanceData& data = inst.As<PerInstanceData>();
    Interpolant output = input;
    output.ClipSpacePosition = input.ClipSpacePosition * data.worldMatrix;
    output.Attributes[0] = data.color;
    return output;
}

Interpolant SequentialVS(const Vertex& input, const ConstantBuffer& cb)
{
    const CbData& data = cb.As<CbData>();
    Interpolant output;
    output.ClipSpacePosition = float4(input.Position.xyz(), 1.0f) * data.modelViewProjection;
    output.Attributes[0] = data.objectColor;
    return output;
}

float4 MainPS(const Interpolant& input, const ConstantBuffer&)
{
    return input.Attributes[0];
}
/////////////////////////////////////////////////////////////////
void CreateCube(VertexBuffer& vb, IndexBuffer& ib)
{
    const float3 positions[8] = {{-0.5f, -0.5f, -0.5f},
                                 {0.5f, -0.5f, -0.5f},
                                 {0.5f, 0.5f, -0.5f},
                                 {-0.5f, 0.5f, -0.5f},
                                 {-0.5f, -0.5f, 0.5f},
                                 {0.5f, -0.5f, 0.5f},
                                 {0.5f, 0.5f, 0.5f},
                                 {-0.5f, 0.5f, 0.5f}};
    const int faces[6][4] = {{1, 5, 6, 2}, {4, 0, 3, 7}, {2, 6, 7, 3}, {4, 5, 1, 0}, {5, 4, 7, 6}, {0, 1, 2, 3}};

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
            else if (wParam == 'I')
                g_useInstancing = !g_useInstancing;
            else if (wParam == 'P')
            {
                PROFILE_STOP_CAPTURE();
                PROFILE_SAVE_CAPTURE("optic_capture");
            }
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
    wc.lpszClassName = "SoftXInstancingDemo";
    RegisterClassEx(&wc);

    RECT rc = {0, 0, (LONG)g_windowSize.x, (LONG)g_windowSize.y};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND wnd = CreateWindowEx(0,
                              "SoftXInstancingDemo",
                              "SoftX Instancing Demo (Async Present)",
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

    std::mt19937 rng(1337);
    const float halfExtent = 50.0f;
    std::uniform_real_distribution<float> dist(-halfExtent, halfExtent);
    std::vector<float4x4> baseWorld(CUBE_COUNT);
    for (int i = 0; i < CUBE_COUNT; ++i)
    {
        float x = dist(rng), y = dist(rng), z = dist(rng);
        baseWorld[i] = translation(float3(x, y, z));
    }

    const uint32_t ringSize = device.GetSwapChainBufferCount();

    std::vector<InstanceBufferHandle> instancesHandles(ringSize);
    for (uint32_t i = 0; i < ringSize; ++i)
    {
        instancesHandles[i] = device.CreateInstanceBuffer();
        device.GetInstanceBuffer(instancesHandles[i])->Reserve(CUBE_COUNT);
    }

    std::vector<CbData> seqCbs(CUBE_COUNT);

    std::vector<PerInstanceData> tempInst(CUBE_COUNT);

    PipelineState psoSeq;
    psoSeq.viewport = Viewport(0, 0, (float)g_windowSize.x, (float)g_windowSize.y, 0.0f, 1.0f);
    psoSeq.rasterizer.cullMode = CullMode::None;
    psoSeq.depth.func = ComparisonFunc::Less;
    psoSeq.depth.writeEnable = true;
    psoSeq.tileSize = TILE_SIZE;
    psoSeq.vertexShader = SequentialVS;
    psoSeq.pixelShader = MainPS;

    PipelineState psoInst = psoSeq;
    psoInst.vertexShader = SharedVS;
    psoInst.instanceShader = InstanceTransform;

    std::vector<CommandAllocator> allocators(ringSize);
    std::vector<std::unique_ptr<CommandList>> lists(ringSize);
    for (uint32_t i = 0; i < ringSize; ++i)
        lists[i] = device.CreateCommandList();

    CommandQueue& queue = device.GetCommandQueue();

    uint2 lastSize = g_windowSize;
    auto lastTime = std::chrono::steady_clock::now();
    g_fpsLastTime = lastTime;

    PROFILE_THREAD("MainThread");
    PROFILE_START_CAPTURE();

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        PROFILE_FRAME("MainThread");
        PROFILE_SCOPE("Frame")
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
        g_cameraAngle += CAMERA_SPEED * dt;

        g_frameCount++;
        float elapsed = std::chrono::duration<float>(now - g_fpsLastTime).count();
        if (elapsed >= 1.0f)
        {
            g_fps = g_frameCount / elapsed;
            g_frameCount = 0;
            g_fpsLastTime = now;

            char title[256];
            snprintf(title,
                     sizeof(title),
                     "SoftX Instancing Demo | Mode: %s | FPS: %.1f | Cubes: %d  [press I]",
                     g_useInstancing ? "Instanced" : "Sequential",
                     g_fps,
                     CUBE_COUNT);
            SetWindowTextA(wnd, title);
        }

        Texture* backBuffer = device.GetTexture(device.GetCurrentBackBufferHandle());
        if (!backBuffer)
            continue;
        uint2 currentSize = backBuffer->Size();
        if (currentSize.x != lastSize.x || currentSize.y != lastSize.y)
        {
            g_windowSize = currentSize;

            device.Release(depthBufferHandle);
            depthBufferHandle = device.CreateDepthBuffer(g_windowSize);

            psoSeq.viewport = Viewport(0, 0, (float)currentSize.x, (float)currentSize.y, 0.0f, 1.0f);
            psoInst.viewport = psoSeq.viewport;
            lastSize = currentSize;
        }

        const uint64_t waitPresent = device.BeginFrame();

        float3 eye = float3(cosf(g_cameraAngle) * CAMERA_RADIUS, CAMERA_HEIGHT, sinf(g_cameraAngle) * CAMERA_RADIUS);
        float4x4 view = look_at(eye, float3(0.0f, 0.0f, 0.0f));
        float4x4 proj =
            perspective(Constants::degrees_to_radians(60.0f), float(g_windowSize.x) / float(g_windowSize.y));
        float4x4 viewProj = view * proj;

        for (int i = 0; i < CUBE_COUNT; ++i)
        {
            tempInst[i].worldMatrix = baseWorld[i] * viewProj;
            tempInst[i].color = float4(0.2f, 0.8f, 0.3f, 1.0f);
        }

        if (!g_useInstancing)
        {
            for (int i = 0; i < CUBE_COUNT; ++i)
            {
                seqCbs[i].modelViewProjection = tempInst[i].worldMatrix;
                seqCbs[i].objectColor = tempInst[i].color;
            }
        }

        device.WaitForPresent(waitPresent);

        const uint32_t slot = device.GetCurrentBackBufferIndex();

        if (g_useInstancing)
        {
            InstanceBuffer* instances = device.GetInstanceBuffer(instancesHandles[slot]);
            instances->Clear();
            for (int i = 0; i < CUBE_COUNT; ++i)
                instances->Add(InstanceData(tempInst[i]));
        }

        allocators[slot].Reset();
        lists[slot]->Reset(allocators[slot]);
        auto& cmd = *lists[slot];

        if (g_useInstancing)
        {
            PROFILE_SCOPE("Instanced draw");
            cmd.SetPipeline(psoInst);
            cmd.SetRenderTarget(device.GetCurrentBackBufferHandle());
            cmd.SetDepthBuffer(depthBufferHandle);
            cmd.SetVertexBuffer(vbHandle);
            cmd.SetIndexBuffer(ibHandle);
            cmd.SetInstanceBuffer(instancesHandles[slot]);
            cmd.Clear(float4(0.05f, 0.10f, 0.18f, 1.0f), 1.0f);
            cmd.DrawIndexedInstanced();
        }
        else
        {
            PROFILE_SCOPE("Sequential draws");
            cmd.SetPipeline(psoSeq);
            cmd.SetRenderTarget(device.GetCurrentBackBufferHandle());
            cmd.SetDepthBuffer(depthBufferHandle);
            cmd.SetVertexBuffer(vbHandle);
            cmd.SetIndexBuffer(ibHandle);
            cmd.Clear(float4(0.05f, 0.10f, 0.18f, 1.0f), 1.0f);
            for (int i = 0; i < CUBE_COUNT; ++i)
            {
                cmd.SetConstantBuffer(ConstantBuffer(seqCbs[i]));
                cmd.DrawIndexed();
            }
        }

        cmd.Close();

        const uint64_t submitFence = queue.Execute(cmd);
        device.Present(submitFence);
    }

    device.WaitForPresents();
    device.WaitIdle();

    device.Release(depthBufferHandle);
    device.Release(vbHandle);
    device.Release(ibHandle);
    for (auto h : instancesHandles)
        device.Release(h);

    for (auto& l : lists)
        l.reset();

    g_device = nullptr;
    return int(msg.wParam);
}
/////////////////////////////////////////////////////////////////
