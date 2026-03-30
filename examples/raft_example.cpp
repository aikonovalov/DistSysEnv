#include <iostream>
#include <vector>

#include "raft/raft.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/event/event.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

namespace distsysenv {

namespace {

Network::Config NetConfig(unsigned seed) {
  NetworkSettings b{
      .drop_prob = 0.0f,
      .min_delay = 0.0f,
      .max_delay = 0.0f,
  };
  return Network::Config{.behavior = b, .random_seed = RandomSeed{seed}};
}

void WireRaftCluster(SimulationScenario& sim, const std::vector<NodeID>& ids,
                     TTime t0) {
  for (size_t i = 0; i < ids.size(); ++i) {
    std::vector<NodeID> peers;
    for (size_t j = 0; j < ids.size(); ++j) {
      if (j != i) {
        peers.push_back(ids[j]);
      }
    }

    peers_list::RequestPayload pl{std::move(peers)};

    sim.ScheduleLocalMessage(
        t0, ids[i], ids[i],
        Message::FromDescription("set_peers", pl.Serialize()));
  }

  for (NodeID id : ids) {
    sim.ScheduleLocalMessage(t0 + 2.0f, id, id,
                             Message::FromDescription("start", {}));
  }
}

struct RaftChecker {
  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedLocalMessageEvent dec{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.msg.GetType() == "raft_state") {
      std::cout << "[CHECKER] Raft state update at time " << ctx.Now()
                << std::endl;
    }
  }
};

}  // namespace

void RunRaftExample() {
  SimulationScenario sim(NetConfig(42));

  sim.AddNode(RaftChecker{}, NodeTag::kCHECKER);

  constexpr int kNumNodes = 3;
  std::vector<NodeID> ids;
  ids.reserve(static_cast<size_t>(kNumNodes));

  for (int i = 0; i < kNumNodes; ++i) {
    ids.push_back(sim.AddNode(RaftNode{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  const TCommand set_cmd(TCommand::Type::eSET, std::string{"key"},
                         std::optional<TVal>{std::string{"value"}});
  sim.ScheduleLocalMessage(
      120.0f, ids[0], ids[0],
      Message::FromDescription("client_command", set_cmd.Serialize()));

  const TCommand get_cmd(TCommand::Type::eGET, std::string{"key"},
                         std::nullopt);
  sim.ScheduleLocalMessage(
      300.0f, ids[1], ids[1],
      Message::FromDescription("client_command", get_cmd.Serialize()));

  const TCommand del_cmd(TCommand::Type::eDEL, std::string{"key"},
                         std::nullopt);
  sim.ScheduleLocalMessage(
      450.0f, ids[2], ids[2],
      Message::FromDescription("client_command", del_cmd.Serialize()));

  sim.RunUntil(900.0f);

  std::cout << "Done" << std::endl;
}

}  // namespace distsysenv

int main() {
  distsysenv::RunRaftExample();
  return 0;
}
