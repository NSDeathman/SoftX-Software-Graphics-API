/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "ResourceHandles.h"
#include <memory>
#include <vector>
#include <cstdint>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

template <typename T, ResourceType Type> class ResourcePool
{
public:
    using Handle = ResourceHandle<Type>;

    Handle Add(std::unique_ptr<T> resource, bool permanent = false)
    {
        if (!freeList.empty())
        {
            uint32_t idx = freeList.back();
            freeList.pop_back();
            slots[idx].resource = std::move(resource);
            slots[idx].alive = true;
            slots[idx].permanent = permanent;
            return Handle{ idx, slots[idx].generation };
        }
        uint32_t idx = static_cast<uint32_t>(slots.size());
        slots.push_back({ std::move(resource), /*generation=*/1, true, permanent });
        return Handle{ idx, 1 };
    }

    T* Get(Handle h)
    {
        if (h.index >= slots.size()) return nullptr;
        auto& s = slots[h.index];
        if (!s.alive || s.generation != h.generation) return nullptr;
        return s.resource.get();
    }

    std::unique_ptr<T> Remove(Handle h, bool force = false)
    {
        if (h.index >= slots.size()) return nullptr;
        auto& s = slots[h.index];
        if (!s.alive || s.generation != h.generation) return nullptr;
        if (s.permanent && !force) return nullptr;

        s.alive = false;
        s.permanent = false;
        s.generation++;
        freeList.push_back(h.index);
        return std::move(s.resource);
    }

    size_t AliveCount() const
    {
        size_t n = 0;
        for (const auto& s : slots) if (s.alive) ++n;
        return n;
    }
    size_t Capacity() const { return slots.size(); }

private:
    struct Slot
    {
        std::unique_ptr<T> resource;
        uint32_t generation;
        bool alive;
        bool permanent;
    };
    std::vector<Slot> slots;
    std::vector<uint32_t> freeList;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
