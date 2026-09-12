#include <mutex>

#include "aircraft.hpp"

class Snapshot
{
private:
    Aircraft _a;
    mutable std::mutex _mtx;

public:
    void write(Aircraft a)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _a = std::move(a);
    }

    Aircraft read() const
    {
        std::lock_guard<std::mutex> lock(_mtx);
        return _a;
    }
};