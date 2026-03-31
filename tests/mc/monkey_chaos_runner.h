#pragma once

#include <algorithm>
#include <cmath>
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

inline constexpr int kMonkeyChaosSetOps = 300;
inline constexpr int kMonkeyChaosGetOps = 300;

struct MonkeyChaosTimeline {
  TTime chaos_t_min = 120.0f;
  TTime chaos_t_max = 4200.0f;
  TTime client_set_t_min = 200.0f;
  TTime client_set_t_max = 4100.0f;
  TTime final_heal_time = 4800.0f;
  TTime verify_get_time = 5200000.0f;
};

inline constexpr MonkeyChaosTimeline kDefaultMonkeyChaosTimeline{};

inline constexpr TTime kDefaultMonkeyChaosRunBeyondVerify = 80000.0f;
inline constexpr TTime kDefaultMonkeyChaosRunUntil =
    kDefaultMonkeyChaosTimeline.verify_get_time +
    kDefaultMonkeyChaosRunBeyondVerify;

struct MonkeyChaosRunOutcome {
  metrics::RaftMcMetricsReport metrics;
  std::optional<std::string> last_ok_set;
  std::optional<std::string> get_value;

  bool Linearizable() const {
    return last_ok_set.has_value() && get_value.has_value() &&
           *get_value == *last_ok_set;
  }
};

inline std::optional<double> McLatencyPercentile(std::vector<TTime> samples,
                                                 double q) {
  if (samples.empty()) {
    return std::nullopt;
  }

  std::sort(samples.begin(), samples.end());
  if (samples.size() == 1) {
    return static_cast<double>(samples.front());
  }

  const double pos = q * static_cast<double>(samples.size() - 1);
  const size_t lo = static_cast<size_t>(std::floor(pos));
  const size_t hi = static_cast<size_t>(std::ceil(pos));
  if (lo >= hi) {
    return static_cast<double>(samples[lo]);
  }

  const double frac = pos - static_cast<double>(lo);
  return (1.0 - frac) * static_cast<double>(samples[lo]) +
         frac * static_cast<double>(samples[hi]);
}

template <typename TRaftNode>
inline MonkeyChaosRunOutcome RunMonkeyChaosQuorumCoreScenario(
    const Network::Config& net, uint64_t kSeed,
    std::string_view data_key_sv = "GOOOOOOL",
    TTime run_until = kDefaultMonkeyChaosRunUntil,
    const MonkeyChaosTimeline& timeline = kDefaultMonkeyChaosTimeline) {
  const std::string data_key(data_key_sv);
  constexpr int kNodes = 5;

  auto schedule_storage =
      std::make_shared<std::vector<metrics::ScheduledClientOp>>();
  auto response_log = std::make_shared<std::vector<command::ResponsePayload>>();
  metrics::RaftMcMetricsCollector metrics(schedule_storage, response_log);

  std::vector<NodeID> ids;
  SimulationScenario sim =
      metrics::BuildRaftMcSimulation<TRaftNode>(net, metrics, kNodes, &ids);

  WireRaftCluster(sim, ids, 1.0f);

  std::vector<bool> is_core(kNodes, false);
  is_core[0] = true;
  is_core[1] = true;
  is_core[2] = true;

  Random chaos_rng{kSeed ^ 525626363ULL};
  QuorumPreservingChaosSchedule chaos(kNodes, is_core, chaos_rng, 48,
                                      timeline.chaos_t_min,
                                      timeline.chaos_t_max);

  const NodeID control = ids[0];
  ApplyQuorumChaosLinkSchedule(sim, ids, control, chaos.link_actions());

  Random client_rng{kSeed ^ 1258726524ULL};
  const std::vector<TTime> set_times =
      RandomClientTimes(client_rng, kMonkeyChaosSetOps,
                        timeline.client_set_t_min, timeline.client_set_t_max);

  MonkeyClientWorkloadSchedule workload =
      ScheduleMonkeySetBurst(sim, ids, client_rng, set_times, data_key, 2);

  *schedule_storage = std::move(workload.ops);

  ApplyQuorumChaosFinalHeal(sim, ids, control, chaos.final_cut(),
                            timeline.final_heal_time);

  TTime after_sets;
  if (schedule_storage->empty()) {
    after_sets = timeline.client_set_t_min;
  } else {
    after_sets = schedule_storage->back().at + 40.0f;
  }

  const TTime get_lower_border =
      std::max(after_sets, timeline.client_set_t_min + 1.0f);

  constexpr TTime kGetHiSlack = 150.0f;
  const TTime get_upper_border = timeline.verify_get_time - kGetHiSlack;

  if (get_lower_border < get_upper_border - 1.0f && kMonkeyChaosGetOps > 1) {
    Random get_rng{kSeed ^ 0xC0FFEEABULL};
    const std::vector<TTime> probe_get_times = RandomClientTimes(
        get_rng, kMonkeyChaosGetOps - 1, get_lower_border, get_upper_border);

    for (TTime t : probe_get_times) {
      ScheduleMonkeyVerifyGet(sim, ids[0], t, data_key, *schedule_storage);
    }
  }

  ScheduleMonkeyVerifyGet(sim, ids[0], timeline.verify_get_time, data_key,
                          *schedule_storage);

  sim.RunUntil(run_until);

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
    os << std::left << std::setw(32) << label << std::right << std::setw(16)
       << a << std::setw(16) << b << '\n';
  };

  const auto fmt_opt4 = [&](const char* label, std::optional<double> a,
                            std::optional<double> b) {
    os << std::left << std::setw(32) << label;
    os << std::right << std::fixed << std::setprecision(4);
    if (a) {
      os << std::setw(16) << *a;
    } else {
      os << std::setw(16) << "n/a";
    }
    if (b) {
      os << std::setw(16) << *b;
    } else {
      os << std::setw(16) << "n/a";
    }
    os << '\n';
  };

  os << "\n--- Monkey chaos metrics: " << left_name << " vs " << right_name
     << " ---\n";
  os << std::left << std::setw(32) << "metric" << std::right << std::setw(16)
     << left_name << std::setw(16) << right_name << '\n';

  os << std::string(64, '-') << '\n';

  fmt_u64("append_entries_rejects", left.append_entries_rejects,
          right.append_entries_rejects);

  fmt_u64("SET samples (ok latencies)", left.set_latencies.size(),
          right.set_latencies.size());

  fmt_u64("GET samples (ok latencies)", left.get_latencies.size(),
          right.get_latencies.size());

  fmt_opt4("mean SET latency", left.MeanSetLatency(), right.MeanSetLatency());

  fmt_opt4("p95 SET latency", McLatencyPercentile(left.set_latencies, 0.95),
           McLatencyPercentile(right.set_latencies, 0.95));

  fmt_opt4("mean GET latency", left.MeanGetLatency(), right.MeanGetLatency());

  fmt_opt4("p95 GET latency", McLatencyPercentile(left.get_latencies, 0.95),
           McLatencyPercentile(right.get_latencies, 0.95));

  os << std::string(64, '-') << '\n';

  os << std::defaultfloat << std::setprecision(6);
}

}  // namespace distsysenv::mc
