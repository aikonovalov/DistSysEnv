#include "context.h"

namespace distsysenv {

SimulationContext::SimulationContext(CoreContext& core) : core_(core) {}

TTime SimulationContext::Now() const { return core_.Now(); }
NodeID SimulationContext::GetOwnID() const {
  return core_.GetOwnID();
}

void SimulationContext::SendMessage(NodeID to, const Message& msg, TTime at_time) {

}
void SendLocal(const Message& msg, TTime at_time) {

}
void SetTimer(std::string name, TTime fire_at) {

}

CoreContext& SimulationContext::Core() { return core_; }


}
