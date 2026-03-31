#pragma once

#include <iomanip>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "../metrics/raft_mc_metrics_collector.h"
#include "apply_quorum_chaos_schedule.h"
#include "quorum_chaos_schedule.h"
#include "raft/message_specs.h"
#include "raft_mc_setup.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

namespace distsysenv::mc {

struct MonkeyChaosRunOutcome {
  metrics::RaftMcMetricsReport metrics;
  std::optional<std::string> last_ok_set;
  std::optional<std::string> get_value;

  bool Linearizable() const {
    return last_ok_set.has_value() && get_value.has_value() &&
           *get_value == *last_ok_set;
  }
};

template <typename TRaftNode>
inline MonkeyChaosRunOutcome RunMonkeyChaosQuorumCoreScenario(
    uint64_t kSeed, std::string_view data_key_sv = "GOOOOOOL") {
  const std::string data_key(data_key_sv);
  constexpr int kNodes = 5;

  auto schedule_storage =
      std::make_shared<std::vector<metrics::ScheduledClientOp>>();
  auto response_log = std::make_shared<std::vector<command::ResponsePayload>>();
  metrics::RaftMcMetricsCollector metrics(schedule_storage, response_log);

  std::vector<NodeID> ids;
  SimulationScenario sim = metrics::BuildRaftMcSimulation<TRaftNode>(
      NetZeroDelay(kSeed), metrics, kNodes, &ids);

  WireRaftCluster(sim, ids, 1.0f);

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

  MonkeyClientWorkloadSchedule workload =
      ScheduleMonkeySetBurst(sim, ids, client_rng, set_times, data_key, 2);

  *schedule_storage = std::move(workload.ops);

  constexpr TTime kHealTime = 4800.0f;
  ApplyQuorumChaosFinalHeal(sim, ids, control, chaos.final_cut(), kHealTime);

  constexpr TTime kVerifyTime = 5200.0f;
  ScheduleMonkeyVerifyGet(sim, ids[0], kVerifyTime, data_key,
                          *schedule_storage);

  sim.RunUntil(35000.0f);

  MonkeyChaosRunOutcome out{.metrics = metrics.SnapshotReport()};

  for (const command::ResponsePayload& r : *response_log) {
    if (r.status != Status::OK || r.command.key() != data_key) {
      continue;
    }

    if (r.command.type() == TCommand::Type::eSET &&
        r.command.value().has_value()) {
      out.last_ok_set = *r.command.value();
    }

    if (r.command.type() == TCommand::Type::eGET && r.value.has_value()) {
      out.get_value = *r.value;
    }
  }

  return out;
}

// This function was made by GPT, I dont want to compose comparsion table by myself
inline void PrintRaftMcMetricsComparison(
    std::ostream& os, std::string_view left_name,
    const metrics::RaftMcMetricsReport& left, std::string_view right_name,
    const metrics::RaftMcMetricsReport& right) {
  const auto fmt_u64 = [&](const char* label, uint64_t a, uint64_t b) {
    os << std::left << std::setw(32) << label << std::right << std::setw(14)
       << a << std::setw(14) << b << '\n';
  };
  const auto fmt_opt = [&](const char* label, std::optional<double> a,
                           std::optional<double> b) {
    os << std::left << std::setw(32) << label;
    os << std::right << std::fixed << std::setprecision(2);
    if (a) {
      os << std::setw(14) << *a;
    } else {
      os << std::setw(14) << "n/a";
    }
    if (b) {
      os << std::setw(14) << *b;
    } else {
      os << std::setw(14) << "n/a";
    }
    os << '\n';
  };

  os << "\n--- Monkey chaos metrics: " << left_name << " vs " << right_name
     << " ---\n";
  os << std::left << std::setw(32) << "metric" << std::right << std::setw(14)
     << left_name << std::setw(14) << right_name << '\n';

  os << std::string(60, '-') << '\n';

  fmt_u64("append_entries_rejects", left.append_entries_rejects,
          right.append_entries_rejects);

  fmt_u64("SET samples (ok latencies)", left.set_latencies.size(),
          right.set_latencies.size());

  fmt_u64("GET samples (ok latencies)", left.get_latencies.size(),
          right.get_latencies.size());

  fmt_opt("mean SET latency", left.MeanSetLatency(), right.MeanSetLatency());

  fmt_opt("mean GET latency", left.MeanGetLatency(), right.MeanGetLatency());

  os << std::string(60, '-') << '\n';

  os << std::defaultfloat << std::setprecision(6);
}

}  // namespace distsysenv::mc
