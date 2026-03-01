#pragma once

#include "../utils/node_id.h"
#include "../utils/message.h"
#include "network_settings.h"
#include <random>

namespace distsysenv {

class Event;
class Context;

class Network {
 public:
  Network(const NetworkSettings& settings, uint64_t rng_seed);

  void HandleEvent(const Event& event, Context& ctx);

 private:
  void HandleMessageSend(NodeID from, NodeID to, const Message& msg, Context& ctx);
  
  bool ShouldDrop(NodeID from, NodeID to);
  
  SimulationClock RandomDelay();

  NetworkSettings settings_;
  std::mt19937 rng_;
};

}  // namespace distsysenv
