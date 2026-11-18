/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
#include "Common.h"
#include "CommandTypes.h"
#include <vector>
/////////////////////////////////////////////////////////////////
SOFTX_BEGIN

class SOFTX_API CommandAllocator
{
public:
    CommandAllocator() = default;
    CommandAllocator(const CommandAllocator&) = delete;
    CommandAllocator& operator=(const CommandAllocator&) = delete;
    CommandAllocator(CommandAllocator&&) noexcept = default;
    CommandAllocator& operator=(CommandAllocator&&) noexcept = default;

    void Reset() { commands.clear(); }

    size_t Size() const { return commands.size(); }
    bool IsEmpty() const { return commands.empty(); }

    const std::vector<Command>& GetCommands() const { return commands; }
    std::vector<Command>& GetCommands() { return commands; }

private:
    std::vector<Command> commands;
};

SOFTX_END
/////////////////////////////////////////////////////////////////
