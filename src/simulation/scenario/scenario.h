#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "../../core/event/event_manager.h"
#include "../../core/message/message.h"
#include "../../core/node_id/node_id.h"
#include "../context/context.h"
#include "../event/event.h"
#include "../network_node/network_node.h"

namespace distsysenv {

enum class NodeTag : uint8_t {
  kCOMMON = 0,
  kNETWORK_GATEWAY = 1,
  kCHECKER = 2,
};

class SimulationScenario {
 public:
  SimulationScenario() = default;

  explicit SimulationScenario(Network::Config network_config);

  EventManager& Manager();
  const EventManager& Manager() const;

  bool HasNetwork() const {
    return simulation_options_.network_gateway.has_value();
  }
  Status NetworkGateway(NodeID* gateway_id) const;

  void SetCheckerSink(NodeID checker_id);

  template <typename T>
  NodeID AddNode(T&& node, NodeTag node_tag = NodeTag::kCOMMON) {
    auto owner =
        std::make_shared<std::remove_cvref_t<T>>(std::forward<T>(node));

    NodeID id =
        manager_.AddNode([this, owner](const Event& e, CoreContext& ctx) {
          SimulationTimerBook& book = timer_books_[ctx.GetOwnID()];
          SimulationContext sim(ctx, simulation_options_, book);
          owner->OnSimulationEvent(e, sim);
        });

    switch (node_tag) {
      case NodeTag::kCOMMON:
        break;

      case NodeTag::kNETWORK_GATEWAY:
        simulation_options_.network_gateway = id;
        break;

      case NodeTag::kCHECKER:
        simulation_options_.checker_local_sink = id;
        break;
    }

    return id;
  }

  void ScheduleMessage(TTime at, NodeID from, NodeID to, Message msg);
  void ScheduleLocalMessage(TTime at, NodeID to, NodeID logical_from,
                            Message msg);

  void ScheduleNodeFail(TTime at, NodeID target, NodeID control_from);
  void ScheduleNodeRecover(TTime at, NodeID target, NodeID control_from);

  void RunUntil(TTime until);

 private:
  EventManager manager_;
  SimulationContextOptions simulation_options_;
  std::unordered_map<NodeID, SimulationTimerBook> timer_books_;
};

}  // namespace distsysenv
