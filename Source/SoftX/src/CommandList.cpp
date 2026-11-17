/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "../include/Exceptions.h"
#include "ThreadUtils.h"
#include "TileGrid.h"
#include "RasterizerCommon.h"
#include "Rasterizer.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

CommandList::CommandList(Device& dev) : device(&dev) {}

CommandList::~CommandList() = default;

void CommandList::Begin()
{
    if (recording)
        SOFTX_THROW(InvalidState("CommandList::Begin called twice"));
    recording = true;
    commands.clear();
}

void CommandList::End()
{
    if (!recording)
        SOFTX_THROW(InvalidState("CommandList::End called without Begin"));
    recording = false;
}

void CommandList::Reset()
{
    commands.clear();
    recording = false;
}

void CommandList::SetPipeline(const PipelineState & pso) { currentPso = pso; }
void CommandList::SetRenderTarget(Texture * rt) { currentRt = rt; }
void CommandList::SetDepthBuffer(DepthBuffer * db) { currentDb = db; }
void CommandList::SetVertexBuffer(VertexBuffer vb) { currentVb = std::move(vb); }
void CommandList::SetIndexBuffer(IndexBuffer ib) { currentIb = std::move(ib); }
void CommandList::SetConstantBuffer(ConstantBuffer cb) { currentCb = std::move(cb); }

void CommandList::Clear(float4 color, float depth)
{
    Command cmd;
    cmd.kind = Command::Kind::Clear;
    cmd.clear.rt = currentRt;
    cmd.clear.db = currentDb;
    cmd.clear.color = color;
    cmd.clear.depth = depth;
    commands.push_back(cmd);
}

void CommandList::Draw()
{
    Command cmd;
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.ib = {};
    cmd.draw.cb = currentCb;
    cmd.draw.indexed = false;
    commands.push_back(cmd);
}

void CommandList::DrawIndexed()
{
    Command cmd;
    cmd.kind = Command::Kind::Draw;
    cmd.draw.pso = currentPso;
    cmd.draw.rt = currentRt;
    cmd.draw.db = currentDb;
    cmd.draw.vb = currentVb;
    cmd.draw.ib = currentIb;
    cmd.draw.cb = currentCb;
    cmd.draw.indexed = true;
    commands.push_back(cmd);
}

void CommandList::Execute()
{
    for (const auto& c : commands)
    {
        switch (c.kind)
        {
        case Command::Kind::Clear: ExecuteClear(c.clear); break;
        case Command::Kind::Draw:  ExecuteDraw(c.draw);  break;
        }
    }
}

std::future<void> CommandList::ExecuteAsync()
{
    return ThreadPoolManager::Get().enqueueBackground([this]() { Execute(); });
}

void CommandList::ExecuteClear(const ClearCommand & c)
{
    if (c.rt) c.rt->Clear(c.color);
    if (c.db) c.db->Clear(c.depth);
}

void CommandList::ExecuteDraw(const DrawCall & d)
{
    if (!d.rt || !d.db || d.vb.IsEmpty())
        SOFTX_THROW(InvalidState("CommandList::Draw missing RT/DB/VB"));

    const size_t vertexCount = d.vb.Size();
    std::vector<Interpolant> transformed(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i)
        transformed[i] = d.pso.vertexShader(d.vb[i], d.cb);

    const size_t indexCount = d.indexed ? d.ib.Size() : vertexCount;
    std::vector<RasterizerCommon::TriangleSetup> setups;
    setups.reserve(indexCount / 3);

    for (size_t i = 0; i + 2 < indexCount; i += 3)
    {
        Interpolant a, b, c;
        if (d.indexed)
        {
            a = transformed[d.ib[i]];
            b = transformed[d.ib[i + 1]];
            c = transformed[d.ib[i + 2]];
        }
        else
        {
            a = transformed[i];
            b = transformed[i + 1];
            c = transformed[i + 2];
        }

        Interpolant clipped[2][3];
        const int numTris = RasterizerCommon::ClipTriangleNearPlane(a, b, c, clipped);
        for (int t = 0; t < numTris; ++t)
        {
            for (int j = 0; j < 3; ++j)
                RasterizerCommon::ClipSpaceToScreenSpace(clipped[t][j], d.pso.viewport);

            auto opt = RasterizerCommon::CreateTriangleSetup(clipped[t][0], clipped[t][1], clipped[t][2], d.pso.rasterizer.cullMode);
            if (opt)
                setups.push_back(std::move(*opt));
        }
    }

    if (setups.empty())
        return;

    TileGrid tileGrid;
    tileGrid.Build(d.rt->Width(), d.rt->Height(), d.pso.tileSize);
    tileGrid.BinTriangles(setups);

    const auto& tiles = tileGrid.GetTiles();
    std::atomic<uint> nextTile{ 0 };
    auto worker = [&]() {
        uint i;
        while ((i = nextTile.fetch_add(1)) < tiles.size())
        {
            for (int setupIdx : tiles[i].triangleIndices)
                Rasterizer::RasterizeTriangle(setups[setupIdx], d.pso, *d.db, *d.rt, d.cb, tiles[i].min, tiles[i].max);
        }
    };
    ThreadUtils::DispatchWorkers(worker);
}

SOFTX_END
/////////////////////////////////////////////////////////////////
