#pragma once

#include <random>
#include "../core/event.h"
#include "../core/event_manager.h"
#include "../node/node_pool.h"
#include "../utils/message.h"
#include "../utils/node_id.h"
#include "../utils/utils.h"
#include "network_settings.h"

namespace distsysenv {

enum class RandomSeed : int32_t {};

class Network {
 public:
  Network(EventManager& event_manager, NodePool& node_pool,
          const NetworkSettings& settings = {},
          RandomSeed rng_seed = RandomSeed{42});

  void OnEvent(const Event& event);

  void OnSendRequest(NodeID from_id, NodeID to_id, Message msg);

 private:
  void HandleMessageReceive(const Event& event);
  void HandleTimer(const Event& event);
  bool ShouldDrop(NodeID from_id, NodeID to_id);
  SimulationClock RandomDelay(NodeID from_id, NodeID to_id);

  EventManager& event_manager_;  // TODO: replace with observer pattern
  NodePool& node_pool_;          // TODO: replace with observer pattern
  NetworkSettings settings_;

  RandomSeed rng_seed_;
  std::mt19937 rng_;
};

}  // namespace distsysenv
