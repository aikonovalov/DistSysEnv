#pragma once

#include <memory>

#include "../core/event/event_handler.h"
#include "context/context.h"

namespace distsysenv {

template <typename T>
EventHandler MakeSimulationHandler(T node,
                                   SimulationContextOptions options = {}) {
  auto owner = std::make_shared<T>(std::move(node));

  return [owner, opts = std::move(options)](const Event& event,
                                            CoreContext& core_ctx) {
    SimulationContext sim(core_ctx, opts);

    owner->OnSimulationEvent(event, sim);
  };
}

}  // namespace distsysenv
