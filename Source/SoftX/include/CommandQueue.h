/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "Fence.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class Device;
class CommandList;

class SOFTX_API CommandQueue
{
public:
    explicit CommandQueue(Device& device);
    ~CommandQueue();

    CommandQueue(const CommandQueue&) = delete;
    CommandQueue& operator=(const CommandQueue&) = delete;

    uint64_t Execute(CommandList& list);
    uint64_t Execute(const std::vector<CommandList*>& lists);
    uint64_t Wait(Fence& fence, uint64_t value);
    uint64_t Signal(Fence& fence);
    uint64_t SignalWithValue(Fence& fence, uint64_t value);

    Fence& GetFence() { return fence; }
    uint64_t GetCompletedValue() const { return fence.GetCompletedValue(); }
    uint64_t GetLastSubmissionValue() const { return nextSubmit.load(); }

    void WaitForFenceValue(uint64_t value) { fence.Wait(value); }
    void WaitIdle();
    bool IsIdle() const;

    Device& GetDevice() const { return *device; }

private:
    uint64_t Enqueue(std::function<void()> task);
    void WorkerProc();

    Device* device = nullptr;
    Fence fence;

    std::thread worker;
    std::deque<std::function<void()>> work;
    mutable std::mutex mutex;
    std::condition_variable condVar;
    bool stop = false;
    std::atomic<uint64_t> nextSubmit{ 0 };
};

SOFTX_END
/////////////////////////////////////////////////////////////////
