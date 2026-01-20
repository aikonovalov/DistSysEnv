#pragma once

#include "../utils/utils.h"
#include "event.h"
#include <queue>
#include <optional>
#include <functional>

namespace distsysenv {

using EventQueue = std::priority_queue<Event, std::vector<Event>, EventEarlier>;

class EventManager {
public:
    SimulationClock Now() const;

    template <typename... Args>
    void Schedule(Args&&... args) {
        queue_.emplace(std::forward<Args>(args)...);
    }

    std::optional<Event> GetNext();

    void Process(const std::function<void(const Event&)>& handler);
    void ProcessUntil(SimulationClock until, const std::function<void(const Event&)>& handler);
    

private:
    SimulationClock now_ = 0;
    EventQueue queue_;
};

}
