#pragma once

#include <optional>
#include <string>

#include "../../core/context/context.h"
#include "../../core/message/message.h"
#include "../../core/node_id/node_id.h"
#include "timer_book.h"

namespace distsysenv {

struct SimulationContextOptions {
  std::optional<NodeID> network_gateway;
  std::optional<NodeID> checker_local_sink;
};

class SimulationContext {
 public:
  explicit SimulationContext(CoreContext& core,
                             const SimulationContextOptions& options,
                             SimulationTimerBook& timer_book);

  TTime Now() const;
  NodeID GetOwnID() const;

  void SendMessage(NodeID to, const Message& msg);

  void SendLocal(const Message& msg);
  void SendLocal(NodeID to, const Message& msg);

  void SetTimer(std::string name, TTime duration);

  void CancelTimer(const std::string& name);

  bool IsTimerValid(const std::string& name,
                    SimulationTimerBook::Token token) const;

  CoreContext& Core();

 private:
  void PushApplicationMessage(NodeID to, const Message& msg);

  CoreContext& core_;
  const SimulationContextOptions& options_;
  SimulationTimerBook& timer_book_;
};

}  // namespace distsysenv
