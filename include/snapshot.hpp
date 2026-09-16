#pragma once

#include <mutex>
#include <optional>

#include "aircraft.hpp"

class Snapshot
{
private:
    // optional because Aircraft has no default ctor, and because "no aircraft
    // selected yet" is a real state at startup that the reader must handle
    std::optional<Aircraft> _a;
    mutable std::mutex _mtx;

public:
    void write(Aircraft a)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _a = std::move(a);
    }

    std::optional<Aircraft> read() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _a;
    }
};