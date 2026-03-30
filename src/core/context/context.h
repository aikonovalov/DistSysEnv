#pragma once

#include "../../utils/time.h"
#include "../../utils/utils.h"
#include "../event/event.h"
#include "../node_id/node_id.h"

namespace distsysenv {

class EventManager;

class CoreContext {
 public:
  CoreContext(EventManager& event_manager, NodeID id);

  void PushEvent(const Event& event);

  TTime Now() const;
  NodeID GetOwnID() const;

 private:
  EventManager& event_manager_;
  NodeID id_;
};

}  // namespace distsysenv
