/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "../include/Exceptions.h"
#include "../include/CommandAllocator.h"
#include "ThreadUtils.h"
#include "TileGrid.h"
#include "RasterizerCommon.h"
#include "Rasterizer.h"
#include <atomic>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

CommandList::CommandList(Device& dev) : device(&dev)
{
}

void CommandList::Reset(CommandAllocator& inAllocator)
{
    if (recording)
        SOFTX_THROW(InvalidState("CommandList::Reset: already recording"));
    if (!inAllocator.IsEmpty())
        SOFTX_THROW(InvalidState("CommandList::Reset: allocator not empty — call allocator.Reset() first"));

    allocator = &inAllocator;
    recording = true;
    closed = false;

    currentPso = PipelineState{};
    currentRt = {};
    currentDb = {};
    currentVb = {};
    currentIb = {};
    currentCb = ConstantBuffer{};
    currentInstanceBuffer = {};
}

void CommandList::Close()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::Close: not recording"));
    recording = false;
    closed = true;
}

void CommandList::SetPipeline(const PipelineState& pso)
{
    currentPso = pso;
}
void CommandList::SetRenderTarget(TextureHandle rt)
{
    currentRt = rt;
}
void CommandList::SetDepthBuffer(DepthBufferHandle db)
{
    currentDb = db;
}
void CommandList::SetVertexBuffer(VertexBufferHandle vb)
{
    currentVb = vb;
}
void CommandList::SetIndexBuffer(IndexBufferHandle ib)
{
    currentIb = ib;
}
void CommandList::SetConstantBuffer(ConstantBuffer cb)
{
    currentCb = std::move(cb);
}
void CommandList::SetInstanceBuffer(InstanceBufferHandle ib)
{
    currentInstanceBuffer = ib;
}

void CommandList::Clear(float4 color, float depth)
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::Clear: not recording"));
    Command cmd{};
    cmd.kind = Command::Kind::Clear;
    cmd.clear.rt = currentRt;
    cmd.clear.db = currentDb;
    cmd.clear.color = color;
    cmd.clear.depth = depth;
    allocator->GetCommands().push_back(std::move(cmd));
}

void CommandList::Draw()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::Draw: not recording"));
    Command cmd{};
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.cb = currentCb;
    cmd.draw.indexed = false;
    cmd.draw.instanced = false;
    allocator->GetCommands().push_back(std::move(cmd));
}

void CommandList::DrawIndexed()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::DrawIndexed: not recording"));
    Command cmd{};
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.ib = currentIb;
    cmd.draw.cb = currentCb;
    cmd.draw.indexed = true;
    cmd.draw.instanced = false;
    allocator->GetCommands().push_back(std::move(cmd));
}

void CommandList::DrawInstanced()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::DrawInstanced: not recording"));
    Command cmd{};
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.cb = currentCb;
    cmd.draw.instanceBuffer = currentInstanceBuffer;
    cmd.draw.indexed = false;
    cmd.draw.instanced = true;
    allocator->GetCommands().push_back(std::move(cmd));
}

void CommandList::DrawIndexedInstanced()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::DrawIndexedInstanced: not recording"));
    Command cmd{};
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.ib = currentIb;
    cmd.draw.cb = currentCb;
    cmd.draw.instanceBuffer = currentInstanceBuffer;
    cmd.draw.indexed = true;
    cmd.draw.instanced = true;
    allocator->GetCommands().push_back(std::move(cmd));
}

void CommandList::ExecuteOnDevice()
{
    PROFILE_SCOPE("CommandList::ExecuteOnDevice")
    if (!allocator)
        SOFTX_THROW(InvalidState("CommandList::ExecuteOnDevice: no allocator bound"));
    for (const auto& c : allocator->GetCommands())
    {
        switch (c.kind)
        {
            case Command::Kind::Clear:
                ExecuteClear(c.clear);
                break;
            case Command::Kind::Draw:
                ExecuteDraw(c.draw);
                break;
        }
    }
}

void CommandList::ExecuteClear(const ClearCommand& c)
{
    PROFILE_SCOPE("CommandList::ExecuteClear")
    Texture* rt = device->GetTexture(c.rt);
    DepthBuffer* db = device->GetDepthBuffer(c.db);
    if (rt)
    {
        PROFILE_SCOPE("Clear target")
        rt->Clear(c.color);
    }
    if (db)
    {
        PROFILE_SCOPE("Clear depth")
        db->Clear(c.depth);
    }
}

void CommandList::ExecuteDraw(const DrawCall& d)
{
    PROFILE_SCOPE("CommandList::ExecuteDraw")
    Texture* rt = device->GetTexture(d.rt);
    DepthBuffer* db = device->GetDepthBuffer(d.db);
    VertexBuffer* vb = device->GetVertexBuffer(d.vb);

    if (!rt || !db || !vb || vb->IsEmpty())
        SOFTX_THROW(InvalidResourceHandle("Draw: missing RT/DB/VB"));

    IndexBuffer* ib = d.indexed ? device->GetIndexBuffer(d.ib) : nullptr;
    InstanceBuffer* instances = d.instanced ? device->GetInstanceBuffer(d.instanceBuffer) : nullptr;

    if (d.indexed && !ib)
        SOFTX_THROW(InvalidResourceHandle("DrawIndexed: missing IB"));
    if (d.instanced && (!instances || !d.pso.instanceShader))
        SOFTX_THROW(InvalidResourceHandle("DrawInstanced: missing instance buffer/shader"));

    const size_t vertexCount = vb->Size();
    const size_t indexCount = d.indexed ? ib->Size() : vertexCount;
    const size_t instanceCount = d.instanced ? instances->Size() : size_t(1);

    std::vector<Interpolant> vertexOutputs(vertexCount);
    {
        PROFILE_SCOPE("Draw::VertexShader")
        for (size_t i = 0; i < vertexCount; ++i)
            vertexOutputs[i] = d.pso.vertexShader((*vb)[i], d.cb);
    }

    std::vector<RasterizerCommon::TriangleSetup> setups;

    std::vector<Interpolant> instVerts;
    if (d.instanced)
        instVerts.resize(vertexCount);

    const std::vector<Interpolant>* activeVerts = &vertexOutputs;

    {
        PROFILE_SCOPE("Draw::SetupTris")

        const size_t triCountPerInstance = indexCount / 3;
        if (triCountPerInstance == 0) return;

        setups.reserve(instanceCount * triCountPerInstance * 2);

        std::vector<Interpolant> instVerts;
        if (d.instanced) instVerts.resize(vertexCount);

        for (size_t inst = 0; inst < instanceCount; ++inst)
        {
            const std::vector<Interpolant>* active = &vertexOutputs;
            if (d.instanced)
            {
                const InstanceData& instData = (*instances)[inst];
                for (size_t i = 0; i < vertexCount; ++i)
                    instVerts[i] = d.pso.instanceShader(vertexOutputs[i], instData);
                active = &instVerts;
            }

            const auto& verts = *active;
            for (size_t tri = 0; tri < triCountPerInstance; ++tri)
            {
                const Interpolant& a = d.indexed ? verts[(*ib)[tri * 3]] : verts[tri * 3];
                const Interpolant& b = d.indexed ? verts[(*ib)[tri * 3 + 1]] : verts[tri * 3 + 1];
                const Interpolant& c = d.indexed ? verts[(*ib)[tri * 3 + 2]] : verts[tri * 3 + 2];

                Interpolant clipped[2][3];
                const int n = RasterizerCommon::ClipTriangleNearPlane(a, b, c, clipped);
                for (int t = 0; t < n; ++t)
                {
                    for (int j = 0; j < 3; ++j)
                        RasterizerCommon::ClipSpaceToScreenSpace(clipped[t][j], d.pso.viewport);
                    auto opt = RasterizerCommon::CreateTriangleSetup(clipped[t][0], clipped[t][1], clipped[t][2], d.pso.rasterizer.cullMode);
                    if (opt) setups.push_back(std::move(*opt));
                }
            }
        }
    }

    if (setups.empty())
        return;

    PROFILE_TAG("Draw.Setups", setups.size())
    PROFILE_TAG("Draw.Instances", instanceCount)

    TileGrid tileGrid;
    {
        PROFILE_SCOPE("Draw::TileGrid::Build+Bin")
        tileGrid.Build(rt->Width(), rt->Height(), d.pso.tileSize);
        tileGrid.BinTriangles(setups);
    }

    const auto& tiles = tileGrid.GetTiles();
    PROFILE_TAG("Draw.Tiles", tiles.size())

    std::atomic<uint> nextTile{ 0 };
    auto worker = [&]
    {
        PROFILE_THREAD("SoftX Raster Worker")
        PROFILE_SCOPE("Draw::Rasterize")
        uint i;
        while ((i = nextTile.fetch_add(1)) < tiles.size())
            for (int setupIdx : tiles[i].triangleIndices)
                Rasterizer::RasterizeTriangle(setups[setupIdx], d.pso, *db, *rt, d.cb,
                    tiles[i].min, tiles[i].max);
    };
    ThreadUtils::DispatchWorkers(worker);
}

SOFTX_END
/////////////////////////////////////////////////////////////////
