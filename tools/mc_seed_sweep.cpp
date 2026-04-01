#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include "hybrid_raft/hybrid_raft.h"
#include "raft/raft.h"
#include "tests/mc/monkey_chaos_runner.h"

using namespace distsysenv;
using namespace distsysenv::mc;

namespace {

constexpr const char* kMcKey = "GOOOOOOL";

constexpr TTime kStressRunUntil = 120000.0f;
constexpr MonkeyChaosTimeline kStressTimeline{
    .chaos_t_min = 2500.0f,
    .chaos_t_max = 72000.0f,
    .client_set_t_min = 3500.0f,
    .client_set_t_max = 68000.0f,
    .final_heal_time = 78000.0f,
    .verify_get_time = 82000.0f,
};

void PrintCsvHeader(std::ostream& os) {
  os << "network,seed,"
        "raft_ae_rej,hyb_ae_rej,"
        "raft_set_n,hyb_set_n,raft_get_n,hyb_get_n,"
        "raft_mean_set,hyb_mean_set,raft_p95_set,hyb_p95_set,"
        "raft_mean_get,hyb_mean_get,raft_p95_get,hyb_p95_get,"
        "raft_linearizable,hyb_linearizable\n";
}

void PrintOpt(std::ostream& os, std::optional<double> v) {
  if (v) {
    os << std::fixed << std::setprecision(6) << *v;
  }
}

void PrintCsvRow(std::ostream& os, std::string_view network, uint64_t seed,
                 const MonkeyChaosRunOutcome& raft,
                 const MonkeyChaosRunOutcome& hyb) {
  const auto& L = raft.metrics;
  const auto& R = hyb.metrics;
  os << network << ',' << seed << ',';
  os << L.append_entries_rejects << ',' << R.append_entries_rejects << ',';
  os << L.set_latencies.size() << ',' << R.set_latencies.size() << ',';
  os << L.get_latencies.size() << ',' << R.get_latencies.size() << ',';
  PrintOpt(os, L.MeanSetLatency());
  os << ',';

  PrintOpt(os, R.MeanSetLatency());
  os << ',';

  PrintOpt(os, McLatencyPercentile(L.set_latencies, 0.95));
  os << ',';

  PrintOpt(os, McLatencyPercentile(R.set_latencies, 0.95));
  os << ',';

  PrintOpt(os, L.MeanGetLatency());
  os << ',';

  PrintOpt(os, R.MeanGetLatency());
  os << ',';

  PrintOpt(os, McLatencyPercentile(L.get_latencies, 0.95));
  os << ',';

  PrintOpt(os, McLatencyPercentile(R.get_latencies, 0.95));
  os << ',';

  os << (raft.Linearizable() ? 1 : 0) << ',' << (hyb.Linearizable() ? 1 : 0)
     << '\n';

  os << std::defaultfloat;
}

void RunPairAndPrint(std::ostream& os, std::string_view network,
                     const Network::Config& net, uint64_t seed) {
  const MonkeyChaosRunOutcome classic =
      RunMonkeyChaosQuorumCoreScenario<RaftNode>(
          net, seed, kMcKey, kStressRunUntil, kStressTimeline);
  const MonkeyChaosRunOutcome hybrid =
      RunMonkeyChaosQuorumCoreScenario<HybridRaftNode>(
          net, seed, kMcKey, kStressRunUntil, kStressTimeline);
  PrintCsvRow(os, network, seed, classic, hybrid);
  os.flush();
}

enum class NetMode { kJitter, kLoss, kBoth };

NetMode ParseMode(const char* s) {
  std::string_view v(s);
  if (v == "loss") {
    return NetMode::kLoss;
  }
  if (v == "both") {
    return NetMode::kBoth;
  }

  return NetMode::kJitter;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: " << argv[0]
              << " <first_seed> <count> [jitter|loss|both]\n";

    return 1;
  }

  const uint64_t first_seed = std::strtoull(argv[1], nullptr, 10);
  const int count = std::atoi(argv[2]);
  if (count <= 0) {
    std::cerr << "count must be positive\n";
    return 1;
  }

  const NetMode mode = (argc >= 4) ? ParseMode(argv[3]) : NetMode::kBoth;

  PrintCsvHeader(std::cout);

  for (int i = 0; i < count; ++i) {
    const uint64_t seed = first_seed + static_cast<uint64_t>(i);
    if (mode == NetMode::kJitter || mode == NetMode::kBoth) {
      RunPairAndPrint(std::cout, "jitter", NetJitterLan(seed), seed);
    }

    if (mode == NetMode::kLoss || mode == NetMode::kBoth) {
      RunPairAndPrint(std::cout, "loss", NetUnstableLan(seed), seed);
    }
  }

  return 0;
}
