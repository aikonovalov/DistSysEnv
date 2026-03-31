#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include "hybrid_raft/hybrid_raft.h"
#include "raft/message_specs.h"
#include "raft/raft.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/event/event.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

namespace distsysenv {

namespace {

Network::Config NetZeroDelay(unsigned seed) {
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

struct CountClientResponses {
  std::shared_ptr<int> count;

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    DecodedLocalMessageEvent dec{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.msg.GetType() == "client_command_response") {
      ++*count;
    }
  }
};

struct LogClientResponses {
  std::shared_ptr<std::vector<command::ResponsePayload>> responses;

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    DecodedLocalMessageEvent dec{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.msg.GetType() == "client_command_response") {
      responses->push_back(
          command::ResponsePayload::Deserialize(dec.msg.GetPayload()));
    }
  }
};

}  // namespace

}  // namespace distsysenv

using namespace distsysenv;

#define DISTSYSENV_RAFT_TEST_NODE_TYPES RaftNode, HybridRaftNode

TEMPLATE_TEST_CASE("Raft single node: election then SET reaches checker",
                   "[raft][simulation]", DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(11));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  ids.push_back(sim.AddNode(TestType{{}}));

  WireRaftCluster(sim, ids, 1.0f);

  const TCommand cmd(TCommand::Type::eSET, std::string{"k"},
                     std::optional<TVal>{std::string{"v"}});
  sim.ScheduleLocalMessage(
      120.0f, ids[0], ids[0],
      Message::FromDescription("client_command", cmd.Serialize()));

  sim.RunUntil(400.0f);

  REQUIRE(*responses >= 1);
}

TEMPLATE_TEST_CASE("Raft single node: GET after SET", "[raft][simulation]",
                   DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(12));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids{sim.AddNode(TestType{{}})};
  WireRaftCluster(sim, ids, 1.0f);

  const TCommand set_cmd(TCommand::Type::eSET, std::string{"x"},
                         std::optional<TVal>{std::string{"1"}});
  const TCommand get_cmd(TCommand::Type::eGET, std::string{"x"}, std::nullopt);

  sim.ScheduleLocalMessage(
      120.0f, ids[0], ids[0],
      Message::FromDescription("client_command", set_cmd.Serialize()));
  sim.ScheduleLocalMessage(
      200.0f, ids[0], ids[0],
      Message::FromDescription("client_command", get_cmd.Serialize()));

  sim.RunUntil(450.0f);

  REQUIRE(*responses >= 2);
}

TEMPLATE_TEST_CASE("Raft three nodes: cluster serves SET", "[raft][simulation]",
                   DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(13));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  for (int i = 0; i < 3; ++i) {
    ids.push_back(sim.AddNode(TestType{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  const TCommand cmd(TCommand::Type::eSET, std::string{"key"},
                     std::optional<TVal>{std::string{"val"}});
  sim.ScheduleLocalMessage(
      250.0f, ids[1], ids[1],
      Message::FromDescription("client_command", cmd.Serialize()));

  sim.RunUntil(800.0f);

  REQUIRE(*responses >= 1);
}

TEMPLATE_TEST_CASE("Raft three nodes: follower forwards command to leader",
                   "[raft][simulation]", DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(14));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  for (int i = 0; i < 3; ++i) {
    ids.push_back(sim.AddNode(TestType{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  const TCommand cmd(TCommand::Type::eGET, std::string{"missing"},
                     std::nullopt);

  sim.ScheduleLocalMessage(
      300.0f, ids[2], ids[2],
      Message::FromDescription("client_command", cmd.Serialize()));

  sim.RunUntil(900.0f);

  REQUIRE(*responses >= 1);
}

TEMPLATE_TEST_CASE("Raft three nodes: majority partition still commits SET",
                   "[raft][simulation][partition]",
                   DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(21));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  for (int i = 0; i < 3; ++i) {
    ids.push_back(sim.AddNode(TestType{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  NodeID gw{Index{0}, Generation{0}};
  REQUIRE(sim.NetworkGateway(&gw) == Status::OK);

  sim.SchedulePartitionPair(400.0f, ids[2], ids[0], true, ids[0]);
  sim.SchedulePartitionPair(400.0f, ids[2], ids[1], true, ids[0]);

  const TCommand cmd(TCommand::Type::eSET, std::string{"pkey"},
                     std::optional<TVal>{std::string{"pval"}});

  sim.ScheduleLocalMessage(
      600.0f, ids[0], ids[0],
      Message::FromDescription("client_command", cmd.Serialize()));

  sim.RunUntil(3000.0f);

  REQUIRE(*responses >= 1);
}

TEMPLATE_TEST_CASE("Raft three nodes: GET observes committed SET value",
                   "[raft][simulation]", DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(15));

  auto log = std::make_shared<std::vector<command::ResponsePayload>>();
  sim.AddNode(LogClientResponses{log}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  for (int i = 0; i < 3; ++i) {
    ids.push_back(sim.AddNode(TestType{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  const TCommand set_cmd(TCommand::Type::eSET, std::string{"chk"},
                         std::optional<TVal>{std::string{"committed"}});
  const TCommand get_cmd(TCommand::Type::eGET, std::string{"chk"},
                         std::nullopt);

  sim.ScheduleLocalMessage(
      280.0f, ids[1], ids[1],
      Message::FromDescription("client_command", set_cmd.Serialize()));
  sim.ScheduleLocalMessage(
      500.0f, ids[2], ids[2],
      Message::FromDescription("client_command", get_cmd.Serialize()));

  sim.RunUntil(1200.0f);

  bool saw_set_ok = false;
  bool saw_get_ok = false;
  for (const command::ResponsePayload& r : *log) {
    if (r.status != Status::OK) {
      continue;
    }
    if (r.command.type() == TCommand::Type::eSET && r.command.key() == "chk") {
      saw_set_ok = true;
    }
    if (r.command.type() == TCommand::Type::eGET && r.command.key() == "chk") {
      REQUIRE(r.value.has_value());
      REQUIRE(*r.value == "committed");
      saw_get_ok = true;
    }
  }
  REQUIRE(saw_set_ok);
  REQUIRE(saw_get_ok);
}

TEMPLATE_TEST_CASE("Raft three nodes: minority partition no client completion",
                   "[raft][simulation][partition]",
                   DISTSYSENV_RAFT_TEST_NODE_TYPES) {
  SimulationScenario sim(NetZeroDelay(22));

  auto responses = std::make_shared<int>(0);
  sim.AddNode(CountClientResponses{responses}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  for (int i = 0; i < 3; ++i) {
    ids.push_back(sim.AddNode(TestType{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  NodeID gw{Index{0}, Generation{0}};
  REQUIRE(sim.NetworkGateway(&gw) == Status::OK);

  sim.SchedulePartitionPair(400.0f, ids[2], ids[0], true, ids[0]);
  sim.SchedulePartitionPair(400.0f, ids[2], ids[1], true, ids[0]);

  const TCommand cmd(TCommand::Type::eSET, std::string{"solo"},
                     std::optional<TVal>{std::string{"nope"}});
  sim.ScheduleLocalMessage(
      700.0f, ids[2], ids[2],
      Message::FromDescription("client_command", cmd.Serialize()));

  sim.RunUntil(2800.0f);

  REQUIRE(*responses == 0);
}
