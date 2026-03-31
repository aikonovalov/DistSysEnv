#include <catch2/catch_test_macros.hpp>

#include <iostream>

#include "hybrid_raft/hybrid_raft.h"
#include "raft/raft.h"

#include "monkey_chaos_runner.h"

namespace distsysenv {

namespace {

constexpr const std::string kMcKey = "GOOOOOOL";

}  // namespace

}  // namespace distsysenv

using namespace distsysenv;
using namespace distsysenv::mc;

TEST_CASE("Raft monkey chaos: quorum core stays connected", "[raft][mc]") {
  constexpr uint64_t kSeed = 25959151;

  const MonkeyChaosRunOutcome outcome =
      RunMonkeyChaosQuorumCoreScenario<RaftNode>(NetJitterLan(kSeed), kSeed,
                                                 kMcKey);

  const auto& report = outcome.metrics;
  REQUIRE(report.MeanSetLatency().has_value());
  REQUIRE(report.MeanGetLatency().has_value());

  REQUIRE(report.set_latencies.size() >= kMonkeyChaosSetOps - 5);
  REQUIRE(report.get_latencies.size() >= kMonkeyChaosGetOps - 5);

  REQUIRE(outcome.Linearizable());
}

namespace {

void RunMetricsComparisonStress(const Network::Config& net, uint64_t kSeed,
                                std::string_view banner) {
  constexpr TTime kRunUntil = 120000.0f;
  constexpr MonkeyChaosTimeline kStressTimeline{
      .chaos_t_min = 2500.0f,
      .chaos_t_max = 72000.0f,
      .client_set_t_min = 3500.0f,
      .client_set_t_max = 68000.0f,
      .final_heal_time = 78000.0f,
      .verify_get_time = 82000.0f,
  };

  const MonkeyChaosRunOutcome classic =
      RunMonkeyChaosQuorumCoreScenario<RaftNode>(net, kSeed, kMcKey, kRunUntil,
                                                 kStressTimeline);
  const MonkeyChaosRunOutcome hybrid =
      RunMonkeyChaosQuorumCoreScenario<HybridRaftNode>(
          net, kSeed, kMcKey, kRunUntil, kStressTimeline);

  std::cout << banner << '\n';
  PrintRaftMcMetricsComparison(std::cout, "Raft", classic.metrics, "HybridRaft",
                               hybrid.metrics);

  REQUIRE(classic.Linearizable());
  REQUIRE(hybrid.Linearizable());

  REQUIRE(classic.metrics.MeanSetLatency().has_value());
  REQUIRE(classic.metrics.MeanGetLatency().has_value());
  REQUIRE(hybrid.metrics.MeanSetLatency().has_value());
  REQUIRE(hybrid.metrics.MeanGetLatency().has_value());
}

}  // namespace

TEST_CASE("Raft vs HybridRaft monkey chaos: metrics (jitter only)",
          "[raft][mc]") {
  constexpr uint64_t kSeed = 4150515345;
  RunMetricsComparisonStress(NetJitterLan(kSeed), kSeed,
                             "--- Monkey chaos: jitter only (no drops) ---");
}

TEST_CASE("Raft vs HybridRaft monkey chaos: metrics (jitter + packet loss)",
          "[raft][mc]") {
  constexpr uint64_t kSeed = 4150515345;
  RunMetricsComparisonStress(NetUnstableLan(kSeed), kSeed,
                             "--- Monkey chaos: jitter + packet loss ---");
}
