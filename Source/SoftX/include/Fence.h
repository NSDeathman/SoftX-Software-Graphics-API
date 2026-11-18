/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API Fence
{
public:
    Fence() = default;
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;

    uint64_t GetCompletedValue() const
    {
        return completed_.load(std::memory_order_acquire);
    }

    void Signal(uint64_t value)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            uint64_t cur = completed_.load(std::memory_order_relaxed);
            if (value > cur)
                completed_.store(value, std::memory_order_release);
        }
        condVar.notify_all();
    }

    void Wait(uint64_t value) const
    {
        if (GetCompletedValue() >= value)
            return;
        std::unique_lock<std::mutex> lock(mutex);
        condVar.wait(lock,
                 [&]
                 {
                     return completed_.load(std::memory_order_acquire) >= value;
                 });
    }

private:
    mutable std::mutex mutex;
    mutable std::condition_variable condVar;
    std::atomic<uint64_t> completed_{0};
};

SOFTX_END
/////////////////////////////////////////////////////////////////
