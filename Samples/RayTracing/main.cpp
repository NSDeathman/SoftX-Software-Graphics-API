/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <map>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <SoftX.h>
#include <AfterMath.h>
/////////////////////////////////////////////////////////////////
using namespace SoftX;
using namespace AfterMath;
/////////////////////////////////////////////////////////////////
Device* g_device = nullptr;
/////////////////////////////////////////////////////////////////
static float g_cameraYaw = 0.0f;
static float g_cameraPitch = 0.0f;
static float g_cameraRadius = 2.0f;
static bool  g_middleButtonDown = false;
static POINT g_lastMousePos = { 0, 0 };
/////////////////////////////////////////////////////////////////
struct Mesh
{
    VertexBuffer vb;
    IndexBuffer ib;

    bool LoadFromOBJ(const std::string& filename)
    {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

        std::filesystem::path fullPath = exeDir / filename;

        std::ifstream file(fullPath.string());
        if (!file.is_open())
        {
            std::string msg = "Model " + filename + " failed to load";
            MessageBox(NULL, msg.c_str(), "Model loading error", MB_OK);
            return false;
        }

        std::vector<float3> positions;
        std::vector<float3> normals;
        std::vector<float2> texcoords;

        std::map<std::tuple<int, int, int>, uint> vertexMap;

        std::string line;
        while (std::getline(file, line))
        {
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos)
                line = line.substr(0, commentPos);

            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "v")
            {
                float x, y, z, w = 1.0f;
                iss >> x >> y >> z;
                if (iss >> w)
                {
                    if (w != 0.0f)
                    {
                        x /= w;
                        y /= w;
                        z /= w;
                    }
                }
                positions.emplace_back(x, y, z);
            }
            else if (type == "vt")
            {
                float u, v= 0.0f;
                iss >> u >> v;
                texcoords.emplace_back(u, v);
            }
            else if (type == "vn")
            {
                float x, y, z;
                iss >> x >> y >> z;
                normals.emplace_back(x, y, z);
            }
            else if (type == "f")
            {
                std::vector<std::tuple<int, int, int>> faceVertices;
                std::string vertexRef;

                while (iss >> vertexRef)
                {
                    int posIdx = 0, texIdx = 0, normIdx = 0;

                    std::stringstream ss(vertexRef);
                    std::string part;
                    std::vector<std::string> parts;
                    while (std::getline(ss, part, '/'))
                        parts.push_back(part);

                    if (parts.empty())
                        continue;

                    posIdx = std::stoi(parts[0]);

                    if (parts.size() > 1 && !parts[1].empty())
                        texIdx = std::stoi(parts[1]);

                    if (parts.size() > 2 && !parts[2].empty())
                        normIdx = std::stoi(parts[2]);

                    if (posIdx < 0) posIdx = (int)positions.size() + posIdx + 1;
                    if (texIdx < 0) texIdx = (int)texcoords.size() + texIdx + 1;
                    if (normIdx < 0) normIdx = (int)normals.size() + normIdx + 1;

                    faceVertices.emplace_back(posIdx - 1, texIdx - 1, normIdx - 1);
                }

                if (faceVertices.size() < 3)
                    continue;

                for (size_t i = 1; i + 1 < faceVertices.size(); ++i)
                {
                    std::tuple<int, int, int> tri[3] = {
                        faceVertices[0],
                        faceVertices[i],
                        faceVertices[i + 1]
                    };

                    for (int j = 0; j < 3; ++j)
                    {
                        auto key = tri[j];
                        auto it = vertexMap.find(key);
                        if (it == vertexMap.end())
                        {
                            Vertex vert;
                            auto [posIdx, texIdx, normIdx] = key;

                            if (posIdx >= 0 && posIdx < (int)positions.size())
                                vert.Position = positions[posIdx];
                            else
                                vert.Position = float3(0.0f, 0.0f, 0.0f);

                            for (int a = 0; a < SOFT_MAX_ATTRIBUTES_COUNT; ++a)
                                vert.Attributes[a] = float4(0.0f, 0.0f, 0.0f, 0.0f);

                            if (normIdx >= 0 && normIdx < (int)normals.size())
                                vert.Attributes[0] = float4(normals[normIdx].x, normals[normIdx].y, normals[normIdx].z, 0.0f);

                            if (texIdx >= 0 && texIdx < (int)texcoords.size())
                                vert.Attributes[1] = float4(texcoords[texIdx].x, texcoords[texIdx].y, 0.0f, 0.0f);

                            uint newIndex = (uint)vb.Size();
                            vb.Add(vert);
                            vertexMap[key] = newIndex;
                            ib.Add(newIndex);
                        }
                        else
                        {
                            ib.Add(it->second);
                        }
                    }
                }
            }
        }

        return true;
    }
};

struct MeshCBuffer
{
    float4x4 worldView;
    float4x4 worldViewProjection;
    float4x4 normalToView;
    float4 color;
    float roughness;
    float metallic;
};

Interpolant DefaultVertexShader(const Vertex& Input, const ConstantBuffer& cb, const TextureTable&)
{
    Interpolant Output;
    const MeshCBuffer* data = reinterpret_cast<const MeshCBuffer*>(cb.Data());
    Output.ClipSpacePosition = float4(Input.Position.xyz(), 1.0f) * data->worldViewProjection;
    Output.Attributes[0] = float4(Input.Position.xyz(), 1.0f) * data->worldView;
    Output.Attributes[1] = normalize(float4(Input.Attributes[0].xyz(), 0.0f) * data->normalToView);
    return Output;
}

float4 DefaultPixelShader(const Interpolant& Input, const ConstantBuffer& cb, const TextureTable&)
{
    const MeshCBuffer* data = reinterpret_cast<const MeshCBuffer*>(cb.Data());
    return data->color;
};

struct Material
{
    float4 color = float4(0.0f);
    float roughness = 1.0f;
    float metallic = 0.0f;
    VertexShader vertexShader = DefaultVertexShader;
    PixelShader pixelShader = DefaultPixelShader;
    bool twoSided = false;
};

struct ObjectPrototype
{
    std::string filePath;
    Mesh mesh;
    Material material;

    bool Load()
    {
        return mesh.LoadFromOBJ(filePath);
    }
};

struct PointLight
{
    float3 position;
    float3 color;
    float brightness;
};

struct Camera
{
    float3 position;
    float3 direction;
    float fov;
    float4x4 view;
    float4x4 projection;
};

struct Scene
{
    struct Node
    {
        std::string name;
        float4x4 transform;
        ObjectPrototype* prototype = nullptr;
        std::vector<std::unique_ptr<Node>> children;

        Node(const std::string& nodeName) : name(nodeName)
        {
            transform = translation(float3(0.0f, 0.0f, 0.0f));
        }

        Node* AddChild(std::unique_ptr<Node> child)
        {
            children.push_back(std::move(child));
            return children.back().get();
        }
    };
    std::unique_ptr<Node> root;
    std::map<std::string, std::unique_ptr<ObjectPrototype>> prototypes;
    std::map<std::string, Camera> cameras;
    Camera* activeCamera = nullptr;

    Scene()
    {
        root = std::make_unique<Node>("Root");
    }
    ~Scene() = default;

    void AddCamera(const std::string& name, const Camera& cam)
    {
        cameras[name] = cam;
        if (!activeCamera)
            activeCamera = &cameras[name];
    }

    void SetActiveCamera(const std::string& name)
    {
        auto it = cameras.find(name);
        if (it != cameras.end())
            activeCamera = &it->second;
    }

    Camera* GetActiveCamera()
    {
        return activeCamera;
    }

    void AddObjectPrototype(const std::string& name, const std::string& filePath, const Material& mat)
    {
        auto prototype = std::make_unique<ObjectPrototype>();
        prototype->filePath = filePath;
        prototype->material = mat;
        if (!prototype->Load())
        {
            MessageBox(NULL, ("Failed to load " + filePath).c_str(), "Error", MB_OK);
            return;
        }
        prototypes[name] = std::move(prototype);
    }

    Node* CreateInstance(const std::string& nodeName, const std::string& prototypeName, Node* parent = nullptr)
    {
        auto node = std::make_unique<Node>(nodeName);

        auto it = prototypes.find(prototypeName);
        if (it != prototypes.end())
            node->prototype = it->second.get();

        Node* resultNode = node.get();

        if (parent)
            parent->AddChild(std::move(node));
        else
            root->AddChild(std::move(node));

        return resultNode;
    }

    void RenderNode(Node* node, DeviceContext& ctx)
    {
        if (node->prototype)
        {
            const auto& prototype = node->prototype;

            MeshCBuffer CBData;
            CBData.color = prototype->material.color;
            CBData.roughness = prototype->material.roughness;
            CBData.metallic = prototype->material.metallic;

            CBData.worldView = node->transform * activeCamera->view;
            CBData.normalToView = transpose(inverse(CBData.worldView));
            CBData.worldViewProjection = CBData.worldView * activeCamera->projection;

            ConstantBuffer cb(&CBData, sizeof(CBData));

            ctx.SetCullMode(CullMode::None);
            ctx.SetDepthFunc(ComparisonFunc::Less);
            ctx.SetFillMode(FillMode::Solid);
            ctx.SetVertexShader(prototype->material.vertexShader);
            ctx.SetPixelShader(prototype->material.pixelShader);
            ctx.SetVertexBuffer(prototype->mesh.vb);
            ctx.SetIndexBuffer(prototype->mesh.ib);
            ctx.SetConstantBuffer(cb);

            ctx.DrawIndexed();
        }

        for (auto& child : node->children)
        {
            RenderNode(child.get(), ctx);
        }
    }

    void Render(DeviceContext& ctx)
    {
        if (!activeCamera) return;

        RenderNode(root.get(), ctx);
    }
};

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
            {
                PresentParameters newParams = g_device->GetPresentParams();
                newParams.BackBufferSize = uint2(newWidth, newHeight);
                g_device->Reset(newParams);

                DeviceContext& ctx = g_device->GetImmediateContext();
                ctx.SetViewport(Viewport(0.0f, 0.0f, newWidth, newHeight, 0.0f, 1.0f));
            }
        }
        return 0;
    }

    case WM_MBUTTONDOWN:
        g_middleButtonDown = true;
        GetCursorPos(&g_lastMousePos);
        SetCapture(hWnd);
        return 0;

    case WM_MBUTTONUP:
        g_middleButtonDown = false;
        ReleaseCapture();
        return 0;

    case WM_MOUSEMOVE:
        if (g_middleButtonDown)
        {
            POINT currentPos;
            GetCursorPos(&currentPos);
            int dx = currentPos.x - g_lastMousePos.x;
            int dy = currentPos.y - g_lastMousePos.y;
            g_lastMousePos = currentPos;

            // Чувствительность вращения (радианы на пиксель)
            const float sensitivity = 0.01f;
            g_cameraYaw += dx * sensitivity;
            g_cameraPitch += dy * sensitivity;

            // Ограничение угла возвышения (чтобы камера не переворачивалась)
            const float maxPitch = 1.55f;  // ~89°
            if (g_cameraPitch > maxPitch) g_cameraPitch = maxPitch;
            if (g_cameraPitch < -maxPitch) g_cameraPitch = -maxPitch;
        }
        return 0;

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g_cameraRadius -= delta * 0.001f * g_cameraRadius;
        if (g_cameraRadius < 0.1f) g_cameraRadius = 0.1f;
        return 0;
    }

    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    uint2 windowSize = uint2(1280, 720);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = "SoftX Demo";
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, (LONG)windowSize.x, (LONG)windowSize.y };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindowEx( 0, "SoftX Demo", "SoftX Demo",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top,
                                nullptr, nullptr, hInstance, nullptr );
    if (!hWnd) return -1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    PresentParameters params;
    params.BackBufferSize = windowSize;
    params.hDeviceWindow = hWnd;
    params.Windowed = true;

    Device device(params);
    g_device = &device;
    DeviceContext& ctx = g_device->GetImmediateContext();

    Scene scene;

    Material blueMetal = Material{ float4(0.017642f, 0.057805f, 0.219526f, 1.0f), 0.236364f, 1.0f };
    Material darkCopper = Material{ float4(0.47932f, 0.171441f, 0.033105f, 1.0f), 0.331818f, 1.0f };
    Material brightCopper = Material{ float4(1.0f, 0.83077f, 0.637597f, 1.0f), 0.331818f, 1.0f };

    scene.AddObjectPrototype("armillary_core_prototype", "armillary_core.obj", blueMetal);
    scene.AddObjectPrototype("armillary_core_supports_prototype", "armillary_core_supports.obj", darkCopper);
    scene.AddObjectPrototype("armillary_ring_big_prototype", "armillary_ring_big.obj", darkCopper);
    scene.AddObjectPrototype("armillary_ring_small_prototype", "armillary_ring_small.obj", brightCopper);

    Scene::Node* coreNode = scene.CreateInstance("armillary_core", "armillary_core_prototype");
    Scene::Node* supportsNode = scene.CreateInstance("armillary_supports", "armillary_core_supports_prototype");

    Scene::Node* ringBig00Node = scene.CreateInstance("armillary_ring_big.00", "armillary_ring_big_prototype");
    ringBig00Node->transform = rotation_euler(-47.6392f, -63.535f, -39.2253f) * scaling(float3(1.0f));
    Scene::Node* ringBig01Node = scene.CreateInstance("armillary_ring_big.01", "armillary_ring_big_prototype");
    ringBig01Node->transform = rotation_euler(32.1426f, 0.0f, -65.1293f) * scaling(float3(1.08274f));
    Scene::Node* ringBig02Node = scene.CreateInstance("armillary_ring_big.02", "armillary_ring_big_prototype");
    ringBig02Node->transform = rotation_euler(205.987f, 13.0251f, 64.4259f) * scaling(float3(1.13502f));

    Scene::Node* ringSmall00Node = scene.CreateInstance("armillary_small_big.00", "armillary_ring_small_prototype");
    ringSmall00Node->transform = rotation_euler(-44.6811f, -62.7455f, 0.0f) * scaling(float3(1.01724f));
    Scene::Node* ringSmall01Node = scene.CreateInstance("armillary_small_big.01", "armillary_ring_small_prototype");
    ringSmall01Node->transform = rotation_euler(25.5851f, 49.3f, 0.0f) * scaling(float3(1.05592f));
    Scene::Node* ringSmall02Node = scene.CreateInstance("armillary_small_big.02", "armillary_ring_small_prototype");
    ringSmall02Node->transform = rotation_euler(-44.6811f, -62.7455f, 95.468f) * scaling(float3(1.17f));

    Camera cam = {};
    cam.position = float3(0.0f, 0.0f, -2.0f);
    cam.direction = float3(0.0f);
    cam.fov = 60;
    scene.AddCamera("camera", cam);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        uint2 resolution = g_device->GetPresentParams().BackBufferSize;
        float aspect = float(resolution.x) / float(resolution.y);

        Camera* activeCam = scene.GetActiveCamera();
        activeCam->view = look_at_spherical(float3(0.0f, 0.0f, 0.0f), g_cameraRadius, g_cameraYaw, g_cameraPitch);
        activeCam->projection = perspective(radians(activeCam->fov), aspect);

        ctx.Clear(ClearFlags::All, float4(1.0f), 1.0f);
        ctx.SetRenderTarget(device.GetBackBuffer(), false);
        ctx.SetViewport(Viewport(0.0f, 0.0f, resolution.x, resolution.y, 0.0f, 1.0f));
        ctx.SetTileSize(resolution.x / 16);

        scene.Render(ctx);

        g_device->Present();
    }

    return 0;
}
/////////////////////////////////////////////////////////////////

