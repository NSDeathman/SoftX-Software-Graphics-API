# SoftX – Software Graphics API

<p align="center">
  <a href="./LICENSE"><img src="https://img.shields.io/badge/license-MIT-green" alt="License"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-23-blue" alt="C++ Standard">
  <a href="https://github.com/NSDeathman/SoftX-Software-Graphics-API/actions/workflows/build.yml">
    <img src="https://github.com/NSDeathman/SoftX-Software-Graphics-API/actions/workflows/build.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/NSDeathman/SoftX-Software-Graphics-API/actions/workflows/daily-tests.yml">
    <img src="https://github.com/NSDeathman/SoftX-Software-Graphics-API/actions/workflows/daily-tests.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/NSDeathman/SoftX-Software-Graphics-API/stargazers">
    <img src="https://img.shields.io/github/stars/NSDeathman/SoftX-Software-Graphics-API?style=social" alt="Stars">
  </a>
  <a href="https://discord.gg/XXvxtnDbBP">
    <img src="https://img.shields.io/discord/308323056592486420?logo=discord&logoColor=white" alt="Discord">
  </a>
</p>

SoftX is a modern, high-performance software graphics API designed for x86/x64 platforms.  
It provides a DirectX‑style programming model with full support for **vertex**, **geometry**, and **pixel shaders**, while leveraging **tile‑based multithreaded rendering** and **SIMD acceleration** for maximum efficiency.

## Key Features

- **DirectX‑inspired pipeline** – familiar concepts: device, immediate/deferred contexts, shaders, constant buffers, texture bindings.
- **Tile‑based rendering** – automatic binning of triangles into screen tiles, enabling efficient parallelization.
- **Multithreading** – create multiple deferred contexts to record command lists concurrently, then execute them in parallel.
- **Shader support** – C++ callable objects (std::function) for vertex, geometry and pixel shaders; easy to integrate custom shading logic.
- **Occlusion query** – hardware‑style asynchronous occlusion queries with per‑draw call visibility results. Backed by a separate SIMD‑optimised query rasteriser that counts visible samples.
- **Flexible vertex format** – Vertex format with custom attributes.
- **MIP‑mapped textures** – textures support a full mip chain with automatic generation (`GenerateMips`), bilinear sampling at arbitrary LOD levels, and configurable sampler state (LOD bias, mip filter, clamp/repeat/mirror wrapping).
- **Hi‑Z depth buffer** – hierarchical depth (Hi‑Z) buffer generation (min/max reduction) for efficient coarse‑grained depth testing and occlusion culling.
- **Console rendering mode** – present the framebuffer as ASCII art directly to the Windows console. Use a configurable character gradient, automatic downsampling, and proper aspect‑ratio correction for crisp text‑mode output.
- **Headless rendering** – render scenes without any visible window, ideal for off‑screen rendering, testing, or server‑side generation. Output can be saved to TGA or used programmatically.
- **Math library included** – **AfterMath**, a HLSL‑friendly mathematics library with row‑major matrices and left‑handed coordinate space (identical to DirectX conventions). Low learning curve for HLSL developers.
- **Dynamic link library** – compiled as a `.lib` import library with a DLL runtime (CRT linked dynamically).
- **Wide compiler support** – compatible with C++14, C++17, and C++20 standards.

## Architecture

SoftX is designed from the ground up for performance and flexibility:

- **Device** – creates resources, owns the backbuffer and depth buffer, manages console output when in ASCII mode.
- **Immediate Context** – the main rendering channel; submits commands directly to the driver (i.e., the software rasteriser).
- **Deferred Contexts** – record rendering commands into command lists that can be executed later on the immediate context; perfect for multithreaded scene generation.
- **Rasteriser** – pluggable backend (scalar, SSE) automatically chosen by `CPUDetector`.
- **Tile‑based renderer** – splits the screen into tiles, bins triangles, and processes tiles in parallel using a thread pool.
- **Occlusion Query** – dedicated query rasteriser runs geometry in a low‑overhead async pass, recording visible sample counts without pixel shading.
- **Depth Buffer** – supports multiple MIP levels (Hi‑Z) for fast hierarchical depth tests, generated on demand.
- **Textures** – `Texture` provides a full mip‑map chain, bilinear filtering, and LOD‑aware sampling through the sampler interface.
- **Shader stages** – user‑supplied functors that follow a simple signature; no separate shader compilation required.

## Examples

### A minimal example of setting up a device and output triangle to the screen

```cpp
#include <SoftX/SoftX.h>

// Create device
PresentParameters params = { {800,600}, hwnd, true };
Device device(params);

// Create vertex buffer
VertexBuffer myVertexBuffer;
myVertexBuffer.Reserve(3);
myVertexBuffer.Add({ float3(-1.0f, -1.0f, 0.0f), float4(1.0f, 0.0f, 0.0f, 0.0f) });
myVertexBuffer.Add({ float3(0.0f, 1.0f, 0.0f), float4(0.0f, 1.0f, 0.0f, 0.0f) });
myVertexBuffer.Add({ float3(1.0f, -1.0f, 0.0f), float4(0.0f, 0.0f, 1.0f, 0.0f) });

// Vertex and pixel shaders (lambdas)
auto vs = [](const Vertex& Input, ConstantBuffer, const TextureTable&) -> Interpolant
{
    Interpolant Output;
    Output.ClipSpacePosition = float4(Input.Position.xyz(), 1.0f);
    Output.Attributes[0] = Input.Attributes[0];
    return Output;
};
auto ps = [](const Interpolant& Input, ConstantBuffer, const TextureTable&) -> float4
{
    return Input.Attributes[0];
};

// Set shaders and resources
auto& ctx = device.GetImmediateContext();
ctx.SetVertexShader(vs);
ctx.SetPixelShader(ps);
ctx.SetVertexBuffer(myVertexBuffer);
ctx.SetRenderTarget(device.GetBackBuffer());

// Draw
ctx.Clear(ClearFlags::All, float4(0,0,0,1), 1.0);
ctx.Draw();
device.Present();
```
<img width="1320" height="802" alt="image" src="https://github.com/user-attachments/assets/e6308fd0-3f6d-484b-98e9-d1c153c353af" />


### Headless rendering
Create a device without a window and save the result to a TGA file.

```cpp
PresentParameters params;
params.BackBufferSize = uint2(800, 600);
params.Headless = true;

Device device(params);
auto& ctx = device.GetImmediateContext();
ctx.SetRenderTarget(device.GetBackBuffer(), true);
ctx.SetViewport(Viewport(0, 0, 800, 600, 0.0f, 1.0f));

// ... set shaders, vertex/index buffers, draw ...
```

### Console (ASCII) rendering
Render into a text console with a character gradient.

```cpp
PresentParameters params;
params.BackBufferSize = uint2(160, 80);   // internal resolution
params.Output         = PresentationMode::Console;
params.ConsoleSize    = uint2(80, 40);    // character grid

Device device(params);
auto& ctx = device.GetImmediateContext();
ctx.SetRenderTarget(device.GetBackBuffer(), true);
ctx.SetViewport(Viewport(0, 0, 160, 80, 0.0f, 1.0f));

while (running) {
    ctx.Clear(ClearFlags::All, float4(0,0,0,1), 1.0f);
    // ... draw ...
    device.Present();   // outputs ASCII art to the console
}
```
<img width="1433" height="862" alt="image" src="https://github.com/user-attachments/assets/f82af2f0-0a73-4206-af65-5539bc0c8261" />

### Occlusion query
Determine which objects are visible without expensive pixel shading.
```cpp
// Create and begin a query
OcclusionQuery query;
query.SetDepthBuffer(ctx.GetDepthBuffer());
query.SetViewport(ctx.GetViewport());
query.SetCullMode(CullMode::Back);
query.SetDepthFunc(ComparisonFunc::Less);
query.Begin();

// Issue draw calls for occludees
query.SetVertexBuffer(cubeVB);
query.SetIndexBuffer(cubeIB);
query.SetVertexShader(OcclusionVS);
query.SetConstantBuffer(cubeCB);
auto id0 = query.DrawIndexed();   // returns query ID for this draw

query.SetVertexBuffer(sphereVB);
query.SetIndexBuffer(sphereIB);
query.SetConstantBuffer(sphereCB);
auto id1 = query.DrawIndexed();

query.End();

// Wait for results (or check periodically with IsReady())
query.Flush();

uint samples0 = 0, samples1 = 0;
bool visible0 = query.GetResult(id0, &samples0);
bool visible1 = query.GetResult(id1, &samples1);

if (visible0) { /* draw cube */ }
if (visible1) { /* draw sphere */ }
```
<img width="1282" height="800" alt="image" src="https://github.com/user-attachments/assets/ee863aa1-eefe-4806-813e-041c063a0dc2" />

## The solution includes the AfterMath math library (header‑only).

Supported platforms: x86 and x64.

## C++ standard: C++14 (minimum), recommended C++17 or C++20.

## License
SoftX is open‑source software released under the MIT License. See LICENSE for details.

For more information, please refer to the CONTRIBUTING guide and the example projects.

<img width="1280" height="644" alt="image" src="https://github.com/user-attachments/assets/c89fe404-5621-4429-8524-4c398c8fead1" />
<img width="1277" height="767" alt="image" src="https://github.com/user-attachments/assets/2ce43b0a-4633-46e0-a9db-697548149b29" />
<img width="1280" height="575" alt="image" src="https://github.com/user-attachments/assets/7c6418e6-8805-4ca0-bb69-bd5d71b9f05a" />
<img width="1280" height="692" alt="image" src="https://github.com/user-attachments/assets/fe3d189a-ed8f-4472-b351-448199613bda" />
<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/3a78586a-91f4-450b-bd63-0112bc924b61" />



