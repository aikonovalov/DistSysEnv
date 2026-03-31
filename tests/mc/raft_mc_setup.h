#pragma once

#include <vector>

#include "../../raft/message_specs.h"
#include "../../raft/raft.h"
#include "../../src/core/message/message.h"
#include "../../src/core/node_id/node_id.h"
#include "../../src/simulation/scenario/scenario.h"
#include "../../src/utils/random.h"

namespace distsysenv::mc {

inline Network::Config NetZeroDelay(uint64_t seed) {
  NetworkSettings b{
      .drop_prob = 0.0f,
      .min_delay = 0.0f,
      .max_delay = 0.0f,
  };

  return Network::Config{.behavior = b, .random_seed = RandomSeed{seed}};
}

inline Network::Config NetUniformMessageDelay(uint64_t seed, TTime min_delay,
                                              TTime max_delay) {
  NetworkSettings b{
      .drop_prob = 0.0f,
      .min_delay = min_delay,
      .max_delay = max_delay,
  };

  return Network::Config{.behavior = b, .random_seed = RandomSeed{seed}};
}

inline Network::Config NetFixedMessageDelay(uint64_t seed, TTime delay) {
  return NetUniformMessageDelay(seed, delay, delay);
}

inline Network::Config NetJitterLan(uint64_t seed) {
  return NetUniformMessageDelay(seed, 1.5f, 14.0f);
}

inline Network::Config NetUnstableLan(uint64_t seed) {
  NetworkSettings b{
      .drop_prob = 0.03f,
      .min_delay = 1.5f,
      .max_delay = 14.0f,
  };

  return Network::Config{.behavior = b, .random_seed = RandomSeed{seed}};
}

inline void WireRaftCluster(SimulationScenario& sim,
                            const std::vector<NodeID>& ids, TTime t0) {
  for (size_t i = 0; i < ids.size(); ++i) {
    std::vector<NodeID> peers;

    for (size_t j = 0; j < ids.size(); ++j) {
      if (j != i) {
        peers.push_back(ids[j]);
      }
    }

    peers_list::RequestPayload req_payload{std::move(peers)};

    sim.ScheduleLocalMessage(
        t0, ids[i], ids[i],
        Message::FromDescription("set_peers", req_payload.Serialize()));
  }

  for (NodeID id : ids) {
    sim.ScheduleLocalMessage(t0 + 2.0f, id, id,
                             Message::FromDescription("start", {}));
  }
}

}  // namespace distsysenv::mc
