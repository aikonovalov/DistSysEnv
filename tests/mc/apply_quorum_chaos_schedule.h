#pragma once

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../../src/core/message/message.h"
#include "../../src/core/node_id/node_id.h"
#include "../../src/simulation/scenario/scenario.h"
#include "../../src/utils/random.h"
#include "../metrics/scheduled_client_op.h"
#include "quorum_chaos_schedule.h"

namespace distsysenv::mc {

inline void ApplyQuorumChaosLinkSchedule(
    SimulationScenario& sim, const std::vector<NodeID>& ids,
    NodeID control_from, const std::vector<ChaosLinkAction>& link_actions) {
  for (const ChaosLinkAction& a : link_actions) {
    const bool isolate = (a.kind == ChaosLinkAction::Kind::kPartition);

    sim.SchedulePartitionPair(a.timestamp, ids[a.i], ids[a.j], isolate,
                              control_from);
  }
}

inline void ApplyQuorumChaosFinalHeal(
    SimulationScenario& sim, const std::vector<NodeID>& ids,
    NodeID control_from, const std::set<std::pair<int, int>>& final_cut,
    TTime heal_time) {
  for (const auto& edge : final_cut) {
    sim.SchedulePartitionPair(heal_time, ids[edge.first], ids[edge.second],
                              false, control_from);
  }
}

struct MonkeyClientWorkloadSchedule {
  std::vector<distsysenv::metrics::ScheduledClientOp> ops;
};

inline MonkeyClientWorkloadSchedule ScheduleMonkeySetBurst(
    SimulationScenario& sim, const std::vector<NodeID>& ids, Random& client_rng,
    const std::vector<TTime>& set_times, const std::string& key,
    int core_index_max_inclusive) {
  MonkeyClientWorkloadSchedule out;
  out.ops.reserve(set_times.size());

  int seq = 0;
  for (TTime t : set_times) {
    const int core_pick = client_rng.uniform<int>(0, core_index_max_inclusive);

    const TCommand cmd(
        TCommand::Type::eSET, key,
        std::optional<TVal>{std::string{"v"} + std::to_string(seq)});

    out.ops.push_back(
        distsysenv::metrics::ScheduledClientOp{.at = t, .command = cmd});

    sim.ScheduleLocalMessage(
        t, ids[core_pick], ids[core_pick],
        Message::FromDescription("client_command", cmd.Serialize()));
    ++seq;
  }

  return out;
}

inline void ScheduleMonkeyVerifyGet(
    SimulationScenario& sim, NodeID target, TTime at, const std::string& key,
    std::vector<distsysenv::metrics::ScheduledClientOp>& out) {
  const TCommand get_cmd(TCommand::Type::eGET, key, std::nullopt);

  out.push_back(
      distsysenv::metrics::ScheduledClientOp{.at = at, .command = get_cmd});

  sim.ScheduleLocalMessage(
      at, target, target,
      Message::FromDescription("client_command", get_cmd.Serialize()));
}

}  // namespace distsysenv::mc
