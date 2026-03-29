#pragma once

#include <optional>
#include <string>

#include "../../core/context/context.h"
#include "../../core/message/message.h"
#include "../../core/node_id/node_id.h"

namespace distsysenv {

struct SimulationContextOptions {
  std::optional<NodeID> network_gateway;
  std::optional<NodeID> checker_local_sink;
};

class SimulationContext {
 public:
  explicit SimulationContext(CoreContext& core,
                             const SimulationContextOptions& options);

  TTime Now() const;
  NodeID GetOwnID() const;

  void SendMessage(NodeID to, const Message& msg);

  void SendLocal(const Message& msg);
  void SendLocal(NodeID to, const Message& msg);

  /// Schedules a one-shot timer after `duration` (simulation time) from now.
  void SetTimer(std::string name, TTime duration);

  CoreContext& Core();

 private:
  void PushApplicationMessage(NodeID to, const Message& msg);

  CoreContext& core_;
  const SimulationContextOptions& options_;
};

}  // namespace distsysenv
