#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

#include "raft/message_specs.h"
#include "raft/raft.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/event/event.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

#include "quorum_chaos_schedule.h"
#include "raft_mc_setup.h"

namespace distsysenv {

namespace {

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

constexpr const std::string kMcKey = "GOOOOOOL";

}  // namespace

}  // namespace distsysenv

using namespace distsysenv;
using namespace distsysenv::mc;

TEST_CASE("Raft monkey chaos: quorum core stays connected", "[raft][mc]") {
  constexpr int kNodes = 5;
  constexpr uint64_t kSeed = 7052005;

  SimulationScenario sim(NetZeroDelay(kSeed));

  auto log = std::make_shared<std::vector<command::ResponsePayload>>();
  sim.AddNode(LogClientResponses{log}, NodeTag::kCHECKER);

  std::vector<NodeID> ids;
  ids.reserve(static_cast<size_t>(kNodes));
  for (int i = 0; i < kNodes; ++i) {
    ids.push_back(sim.AddNode(RaftNode{{}}));
  }

  WireRaftCluster(sim, ids, 1.0f);

  NodeID gw{Index{0}, Generation{0}};
  REQUIRE(sim.NetworkGateway(&gw) == Status::OK);

  std::vector<bool> is_core(static_cast<size_t>(kNodes), false);
  is_core[0] = true;
  is_core[1] = true;
  is_core[2] = true;

  Random chaos_rng{kSeed ^ 525626363ULL};
  QuorumPreservingChaosSchedule chaos(kNodes, is_core, chaos_rng, 48, 120.0f,
                                      4200.0f);

  const NodeID control = ids[0];
  for (const ChaosLinkAction& a : chaos.link_actions()) {
    const bool isolate = (a.kind == ChaosLinkAction::Kind::kPartition);

    sim.SchedulePartitionPair(a.timestamp, ids[static_cast<size_t>(a.i)],
                              ids[static_cast<size_t>(a.j)], isolate, control);
  }

  Random client_rng{kSeed ^ 1258726524ULL};
  const std::vector<TTime> set_times =
      RandomClientTimes(client_rng, 18, 200.0f, 4100.0f);

  int seq = 0;
  for (TTime t : set_times) {
    const int core_pick = client_rng.uniform<int>(0, 2);

    const TCommand cmd(TCommand::Type::eSET, std::string{kMcKey},
                       std::string{"v"} + std::to_string(seq));

    sim.ScheduleLocalMessage(
        t, ids[static_cast<size_t>(core_pick)],
        ids[static_cast<size_t>(core_pick)],
        Message::FromDescription("client_command", cmd.Serialize()));
    ++seq;
  }

  constexpr TTime kHealTime = 4800.0f;
  for (const auto& edge : chaos.final_cut()) {
    sim.SchedulePartitionPair(kHealTime, ids[static_cast<size_t>(edge.first)],
                              ids[static_cast<size_t>(edge.second)], false,
                              control);
  }

  constexpr TTime kVerifyTime = 5200.0f;
  const TCommand get_cmd(TCommand::Type::eGET, std::string{kMcKey},
                         std::nullopt);

  sim.ScheduleLocalMessage(
      kVerifyTime, ids[0], ids[0],
      Message::FromDescription("client_command", get_cmd.Serialize()));

  sim.RunUntil(35000.0f);

  std::optional<std::string> last_ok_set;
  for (const command::ResponsePayload& r : *log) {
    if (r.status != Status::OK) {
      continue;
    }

    if (r.command.type() == TCommand::Type::eSET && r.command.key() == kMcKey) {
      REQUIRE(r.command.value().has_value());

      last_ok_set = *r.command.value();
    }
  }

  REQUIRE(last_ok_set.has_value());

  std::optional<std::string> get_value;
  for (const command::ResponsePayload& r : *log) {
    if (r.status != Status::OK) {
      continue;
    }

    if (r.command.type() == TCommand::Type::eGET && r.command.key() == kMcKey) {
      REQUIRE(r.value.has_value());

      get_value = *r.value;
    }
  }

  REQUIRE(get_value.has_value());
  REQUIRE(*get_value == *last_ok_set);
}
