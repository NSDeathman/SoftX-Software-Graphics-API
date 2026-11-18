/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "../include/SoftX.h"
#include "../include/CommandQueue.h"
#include "../include/CommandList.h"
#include "../include/Exceptions.h"
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

CommandQueue::CommandQueue(Device& device) : device(&device)
{
    worker = std::thread([this] { WorkerProc(); });
}

CommandQueue::~CommandQueue()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop = true;
    }
    condVar.notify_all();
    if (worker.joinable())
        worker.join();
}

uint64_t CommandQueue::Enqueue(std::function<void()> task)
{
    PROFILE_SCOPE("CommandQueue::Enqueue")
    uint64_t fenceValue;
    {
        std::lock_guard<std::mutex> lock(mutex);
        fenceValue = ++nextSubmit;
        work.push_back([this, task = std::move(task), fenceValue]() mutable 
        {
            task();
            fence.Signal(fenceValue);
        });
    }
    condVar.notify_one();
    return fenceValue;
}

uint64_t CommandQueue::Execute(CommandList& list)
{
    PROFILE_SCOPE("CommandQueue::Execute")

    if (!list.IsClosed())
        SOFTX_THROW(InvalidState("CommandQueue::Execute: command list must be closed"));

    CommandList* ptr = &list;
    return Enqueue([ptr] { ptr->ExecuteOnDevice(); });
}

uint64_t CommandQueue::Execute(const std::vector<CommandList*>& lists)
{
    PROFILE_SCOPE("CommandQueue::Execute")

    for (auto* l : lists)
        if (!l || !l->IsClosed())
            SOFTX_THROW(InvalidState("CommandQueue::Execute(batch): all lists must be closed"));

    auto copy = lists;
    return Enqueue([copy = std::move(copy)]{
        for (auto* l : copy)
            l->ExecuteOnDevice();
        });
}

uint64_t CommandQueue::Wait(Fence& fence, uint64_t value)
{
    PROFILE_SCOPE("CommandQueue::Wait")
    return Enqueue([&fence, value] { fence.Wait(value); });
}

uint64_t CommandQueue::Signal(Fence& fence)
{
    PROFILE_SCOPE("CommandQueue::Signal")
    return Enqueue([&fence, this] 
        {
            fence.Signal(nextSubmit.load(std::memory_order_relaxed));
        });
}

uint64_t CommandQueue::SignalWithValue(Fence& fence, uint64_t value)
{
    PROFILE_SCOPE("CommandQueue::SignalWithValue")
    return Enqueue([&fence, value] { fence.Signal(value); });
}

void CommandQueue::WaitIdle()
{
    PROFILE_SCOPE("CommandQueue::WaitIdle")
    uint64_t target = nextSubmit.load();
    if (target > 0)
        fence.Wait(target);
}

bool CommandQueue::IsIdle() const
{
    return fence.GetCompletedValue() >= nextSubmit.load();
}

void CommandQueue::WorkerProc()
{
    PROFILE_THREAD("SoftX CommandQueue Worker")
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condVar.wait(lock, [&] { return stop || !work.empty(); });
            if (stop && work.empty())
                return;
            task = std::move(work.front());
            work.pop_front();
        }

        PROFILE_SCOPE("CommandQueue::Task")
        try { task(); }
        catch (...) { }
    }
}

SOFTX_END
/////////////////////////////////////////////////////////////////
