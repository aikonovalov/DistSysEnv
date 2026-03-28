#pragma once

#include <cstdint>
#include <unordered_map>

#include "../../core/event/event_handler.h"
#include "../../utils/random.h"
#include "../../utils/time.h"
#include "../event/event.h"
#include "network_settings.h"

namespace distsysenv {

class CoreContext;
struct Event;

class Network {
 public:
  struct Config {
    NetworkSettings behavior{};
    RandomSeed random_seed{};
  };

  explicit Network(Config config);

  void OnEvent(const Event& event, CoreContext& ctx);

 private:
  void HandleSend(NodeID from, NodeID to, const Message& msg, CoreContext& ctx);
  void SetNodeStatus(NodeStatusEventPayload::Status status, NodeID node_id);

  bool ShouldDrop(NodeID from, NodeID to);

  TTime RandomDelay();

  std::unordered_map<NodeID, NodeNetworkSettings> node_settings_;

  NetworkSettings settings_;
  Random rng_;
};

EventHandler MakeNetworkHandler(Network::Config config);

}  // namespace distsysenv
