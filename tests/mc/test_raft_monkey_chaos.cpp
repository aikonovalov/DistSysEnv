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

#include "../metrics/raft_mc_metrics_collector.h"
#include "apply_quorum_chaos_schedule.h"
#include "quorum_chaos_schedule.h"
#include "raft_mc_setup.h"

namespace distsysenv {

namespace {

constexpr const std::string kMcKey = "GOOOOOOL";

}  // namespace

}  // namespace distsysenv

using namespace distsysenv;
using namespace distsysenv::mc;
using namespace distsysenv::metrics;

TEST_CASE("Raft monkey chaos: quorum core stays connected", "[raft][mc]") {
  constexpr int kNodes = 5;
  constexpr uint64_t kSeed = 7052005;

  auto schedule_storage = std::make_shared<std::vector<ScheduledClientOp>>();
  auto response_log = std::make_shared<std::vector<command::ResponsePayload>>();
  RaftMcMetricsCollector metrics(schedule_storage, response_log);

  std::vector<NodeID> ids;
  SimulationScenario sim =
      BuildRaftMcSimulation<RaftNode>(NetZeroDelay(kSeed), metrics, kNodes, &ids);

  WireRaftCluster(sim, ids, 1.0f);

  NodeID gw{Index{0}, Generation{0}};
  REQUIRE(sim.NetworkGateway(&gw) == Status::OK);

  std::vector<bool> is_core(kNodes, false);
  is_core[0] = true;
  is_core[1] = true;
  is_core[2] = true;

  Random chaos_rng{kSeed ^ 525626363ULL};
  QuorumPreservingChaosSchedule chaos(kNodes, is_core, chaos_rng, 48, 120.0f,
                                      4200.0f);

  const NodeID control = ids[0];
  ApplyQuorumChaosLinkSchedule(sim, ids, control, chaos.link_actions());

  Random client_rng{kSeed ^ 1258726524ULL};
  const std::vector<TTime> set_times =
      RandomClientTimes(client_rng, 18, 200.0f, 4100.0f);

  MonkeyClientWorkloadSchedule workload = ScheduleMonkeySetBurst(
      sim, ids, client_rng, set_times, kMcKey, 2);

  *schedule_storage = std::move(workload.ops);

  constexpr TTime kHealTime = 4800.0f;
  ApplyQuorumChaosFinalHeal(sim, ids, control, chaos.final_cut(), kHealTime);

  constexpr TTime kVerifyTime = 5200.0f;
  ScheduleMonkeyVerifyGet(sim, ids[0], kVerifyTime, kMcKey, *schedule_storage);

  sim.RunUntil(35000.0f);

  const RaftMcMetricsReport report = metrics.SnapshotReport();
  REQUIRE(report.MeanSetLatency().has_value());
  REQUIRE(report.MeanGetLatency().has_value());
  REQUIRE(report.set_latencies.size() >= 1);
  REQUIRE(report.get_latencies.size() == 1);

  std::optional<std::string> last_ok_set;
  for (const command::ResponsePayload& r : *response_log) {
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
  for (const command::ResponsePayload& r : *response_log) {
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
