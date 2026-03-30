#include "context.h"
#include "../event/event.h"
#include "../event/event_manager.h"

namespace distsysenv {

CoreContext::CoreContext(EventManager& event_manager, NodeID id)
    : event_manager_(event_manager), id_(id) {}

void CoreContext::PushEvent(const Event& event) {
  event_manager_.PushEvent(event);
}

TTime CoreContext::Now() const {
  return event_manager_.Now();
}

NodeID CoreContext::GetOwnID() const {
  return id_;
}

}  // namespace distsysenv
