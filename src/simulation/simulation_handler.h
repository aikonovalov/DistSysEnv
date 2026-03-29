#pragma once

#include <memory>
#include <utility>

#include "../core/event/event_handler.h"
#include "context/context.h"

namespace distsysenv {

template <typename T>
EventHandler MakeSimulationHandler(T node,
                                   SimulationContextOptions options = {}) {
  auto owner = std::make_shared<T>(std::move(node));
  auto book = std::make_unique<SimulationTimerBook>();

  return [owner, opts = std::move(options),
          book = std::move(book)](const Event& event, CoreContext& core_ctx) {
    SimulationContext sim(core_ctx, opts, *book);
    
    owner->OnSimulationEvent(event, sim);
  };
}

}  // namespace distsysenv
