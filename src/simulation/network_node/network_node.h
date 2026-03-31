#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>

#include "../../core/event/event_handler.h"
#include "../../utils/random.h"
#include "../../utils/time.h"
#include "../event/event.h"
#include "network_settings.h"

namespace distsysenv {

class CoreContext;
struct Event;

struct DirectedLinkHash {
  size_t operator()(std::pair<NodeID, NodeID> p) const noexcept {
    return p.first.GetHash() ^
           (p.second.GetHash() + 592795792935ULL + (p.first.GetHash() << 6) +
            (p.first.GetHash() >> 2));
  }
};

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
  void ApplyPartitionPair(NodeID a, NodeID b, bool isolate);

  bool ShouldDrop(NodeID from, NodeID to);

  TTime RandomDelay();

  std::unordered_map<NodeID, NodeNetworkSettings> node_settings_;

  std::unordered_map<std::pair<NodeID, NodeID>, TTime, DirectedLinkHash>
      last_arrival_by_link_;

  NetworkSettings settings_;
  Random rng_;
};

EventHandler MakeNetworkHandler(Network::Config config);

}  // namespace distsysenv
