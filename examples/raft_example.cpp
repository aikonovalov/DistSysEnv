#include <iostream>

#include "../raft/message_specs.h"
#include "../raft/raft.h"
#include "../src/core/message/message.h"
#include "../src/core/node_id/node_id.h"
#include "../src/simulation/event/event.h"
#include "../src/simulation/scenario/scenario.h"
#include "../src/utils/random.h"

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
  int next_seq = 0;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedLocalMessageEvent dec{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    const int seq = ++next_seq;
    const TTime t = ctx.Now();
    const int64_t from_idx = GetIndexVal(dec.from.index());

    if (dec.msg.GetType() == "raft_state") {
      const Bytes& pl = dec.msg.GetPayload();
      if (pl.empty()) {
        return;
      }
      const auto b = static_cast<uint8_t>(pl[0]);
      const char* role = "?";
      if (b == 0) {
        role = "FOLLOWER";
      } else if (b == 1) {
        role = "CANDIDATE";
      } else if (b == 2) {
        role = "LEADER";
      }
      std::cout << "[CHECKER] #" << seq << " raft_state t=" << t
                << " node=" << from_idx << " -> " << role << std::endl;
      return;
    }

    if (dec.msg.GetType() == "client_command_response") {
      command::ResponsePayload resp =
          command::ResponsePayload::Deserialize(dec.msg.GetPayload());
      const char* st = (resp.status == Status::OK) ? "OK" : "ERROR";
      const char* ty = "?";
      switch (resp.command.type()) {
        case TCommand::Type::eGET:
          ty = "GET";
          break;
        case TCommand::Type::eSET:
          ty = "SET";
          break;
        case TCommand::Type::eDEL:
          ty = "DEL";
          break;
      }
      std::cout << "[CHECKER] #" << seq << " client_response t=" << t
                << " node=" << from_idx << " " << ty
                << " key=" << resp.command.key() << " status=" << st;
      if (resp.command.type() == TCommand::Type::eGET &&
          resp.value.has_value()) {
        std::cout << " value=" << *resp.value;
      }
      std::cout << std::endl;
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
