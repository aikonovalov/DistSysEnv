#pragma once

#include "../../core/context/context.h"
#include "../../core/message/message.h"

namespace distsysenv {

class SimulationContext {
public:
  explicit SimulationContext(CoreContext& core);

  TTime Now() const;
  NodeID GetOwnID() const;

  void SendMessage(NodeID to, const Message& msg, TTime at_time);
  void SendLocal(const Message& msg, TTime at_time);
  void SetTimer(std::string name, TTime fire_at);

  CoreContext& Core();

private:
  CoreContext& core_;
};

}
