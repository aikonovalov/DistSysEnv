#pragma once

#include <random>
#include <unordered_map>
#include "../core/message/message.h"
#include "../core/node_id/node_id.h"
#include "../utils/time.h"
#include "network_settings.h"

namespace distsysenv {

class Event;
class Context;

class Network {
 public:
  Network(const NetworkSettings& settings, uint64_t rng_seed);

  void Event(const Event& event, Context& ctx);

 private:
  void HandleMessageSend(NodeID from, NodeID to, const Message& msg,
                         Context& ctx);
  void HandleNodeFail(NodeID node_id);
  void HandleNodeRecover(NodeID node_id);

  bool ShouldDrop(NodeID from, NodeID to);

  TTime RandomDelay();

  std::unordered_map<NodeID, NodeNetworkSettings> node_settings_;

  NetworkSettings settings_;
  std::mt19937 rng_;
};

}  // namespace distsysenv
