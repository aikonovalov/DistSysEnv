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
  constexpr uint64_t kSeed = 7052005;

  const MonkeyChaosRunOutcome outcome =
      RunMonkeyChaosQuorumCoreScenario<RaftNode>(kSeed, kMcKey);

  const auto& report = outcome.metrics;
  REQUIRE(report.MeanSetLatency().has_value());
  REQUIRE(report.MeanGetLatency().has_value());
  REQUIRE(report.set_latencies.size() >= 1);
  REQUIRE(report.get_latencies.size() == 1);

  REQUIRE(outcome.Linearizable());
}

TEST_CASE("Raft vs HybridRaft monkey chaos: metrics comparison", "[raft][mc]") {
  constexpr uint64_t kSeed = 7052005;

  const MonkeyChaosRunOutcome classic =
      RunMonkeyChaosQuorumCoreScenario<RaftNode>(kSeed, kMcKey);
  const MonkeyChaosRunOutcome hybrid =
      RunMonkeyChaosQuorumCoreScenario<HybridRaftNode>(kSeed, kMcKey);

  PrintRaftMcMetricsComparison(std::cout, "Raft", classic.metrics, "HybridRaft",
                               hybrid.metrics);

  REQUIRE(classic.Linearizable());
  REQUIRE(hybrid.Linearizable());

  REQUIRE(classic.metrics.MeanSetLatency().has_value());
  REQUIRE(classic.metrics.MeanGetLatency().has_value());
  REQUIRE(hybrid.metrics.MeanSetLatency().has_value());
  REQUIRE(hybrid.metrics.MeanGetLatency().has_value());
}
