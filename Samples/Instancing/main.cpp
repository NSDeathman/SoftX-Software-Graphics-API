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
                              "SoftX Instancing Demo",
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

    // ---- device ----
    DeviceConfig config;
    config.backBufferSize = g_windowSize;
    config.mode = PresentationMode::Windowed;
    config.hwnd = wnd;
    Device device(config);
    g_device = &device;

    auto depthBuffer = device.CreateDepthBuffer(g_windowSize);

    VertexBuffer vb;
    IndexBuffer ib;
    CreateCube(vb, ib);

    std::mt19937 rng(1337);
    const float halfExtent = 50.0f;
    std::uniform_real_distribution<float> dist(-halfExtent, halfExtent);
    std::vector<float4x4> baseWorld(CUBE_COUNT);
    for (int i = 0; i < CUBE_COUNT; ++i)
    {
        float x = dist(rng);
        float y = dist(rng);
        float z = dist(rng);
        baseWorld[i] = translation(float3(x, y, z));
    }

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

    InstanceBuffer instances;
    instances.Reserve(CUBE_COUNT);
    std::vector<CbData> seqCbs(CUBE_COUNT);

    CommandList cmd(device);
    uint2 lastSize = g_windowSize;

    auto lastTime = std::chrono::steady_clock::now();
    g_fpsLastTime = lastTime;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
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

        uint2 currentSize = device.GetBackBuffer()->Size();
        if (currentSize.x != lastSize.x || currentSize.y != lastSize.y)
        {
            g_windowSize = currentSize;
            depthBuffer = device.CreateDepthBuffer(g_windowSize);
            psoSeq.viewport = Viewport(0, 0, (float)currentSize.x, (float)currentSize.y, 0.0f, 1.0f);
            psoInst.viewport = psoSeq.viewport;
            lastSize = currentSize;
        }

        float3 eye = float3(cosf(g_cameraAngle) * CAMERA_RADIUS, CAMERA_HEIGHT, sinf(g_cameraAngle) * CAMERA_RADIUS);
        float4x4 view = look_at(eye, float3(0.0f, 0.0f, 0.0f));
        float4x4 proj = perspective(Constants::degrees_to_radians(60.0f), float(g_windowSize.x) / float(g_windowSize.y));
        float4x4 viewProj = view * proj;

        if (g_useInstancing)
        {
            instances.Clear();
            for (int i = 0; i < CUBE_COUNT; ++i)
            {
                PerInstanceData data;
                data.worldMatrix = baseWorld[i] * viewProj;
                data.color = float4(0.2f, 0.8f, 0.3f, 1.0f);
                instances.Add(InstanceData(data));
            }
        }
        else
        {
            for (int i = 0; i < CUBE_COUNT; ++i)
            {
                seqCbs[i].modelViewProjection = baseWorld[i] * viewProj;
                seqCbs[i].objectColor = float4(0.2f, 0.8f, 0.3f, 1.0f);
            }
        }

        cmd.Begin();

        if (g_useInstancing)
        {
            PROFILE_SCOPE("Instanced draw");
            cmd.SetPipeline(psoInst);
            cmd.SetRenderTarget(device.GetBackBuffer());
            cmd.SetDepthBuffer(depthBuffer.get());
            cmd.SetVertexBuffer(vb);
            cmd.SetIndexBuffer(ib);
            cmd.SetInstanceBuffer(instances);
            cmd.Clear(float4(0.05f, 0.10f, 0.18f, 1.0f), 1.0f);
            cmd.DrawIndexedInstanced();
        }
        else
        {
            PROFILE_SCOPE("Sequential draws");
            cmd.SetPipeline(psoSeq);
            cmd.SetRenderTarget(device.GetBackBuffer());
            cmd.SetDepthBuffer(depthBuffer.get());
            cmd.SetVertexBuffer(vb);
            cmd.SetIndexBuffer(ib);
            cmd.Clear(float4(0.05f, 0.10f, 0.18f, 1.0f), 1.0f);
            for (int i = 0; i < CUBE_COUNT; ++i)
            {
                cmd.SetConstantBuffer(ConstantBuffer(seqCbs[i]));
                cmd.DrawIndexed();
            }
        }

        cmd.End();
        cmd.Execute();

        device.Present();
    }

    g_device = nullptr;
    return int(msg.wParam);
}
/////////////////////////////////////////////////////////////////
