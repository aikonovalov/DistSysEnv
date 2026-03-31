#pragma once

#include <algorithm>
#include <set>
#include <utility>
#include <vector>
#include "../../src/utils/random.h"
#include "../../src/utils/time.h"

namespace distsysenv::mc {

inline std::pair<int, int> NormPair(int i, int j) {
  return std::pair<int, int>{std::min(i, j), std::max(i, j)};
}

struct ChaosLinkAction {
  enum class Kind : uint8_t { kPartition = 0, kHeal = 1 } kind;
  int i = 0;
  int j = 0;
  TTime timestamp = 0;
};

class QuorumPreservingChaosSchedule {
 public:
  QuorumPreservingChaosSchedule(int num_nodes, const std::vector<bool>& is_core,
                                Random& rng, int num_link_events, TTime t_min,
                                TTime t_max);

  const std::vector<ChaosLinkAction>& link_actions() const {
    return link_actions_;
  }

  const std::set<std::pair<int, int>>& final_cut() const { return final_cut_; }

 private:
  std::vector<ChaosLinkAction> link_actions_;
  std::set<std::pair<int, int>> final_cut_;
};

inline QuorumPreservingChaosSchedule::QuorumPreservingChaosSchedule(
    int num_nodes, const std::vector<bool>& is_core, Random& rng,
    int num_link_events, TTime t_min, TTime t_max)
    : final_cut_{} {
  std::set<std::pair<int, int>> cut;

  constexpr int kMaxPickAttempts = 64;
  for (int e = 0; e < num_link_events; ++e) {
    const bool can_heal = !cut.empty();
    const bool want_heal = can_heal && rng.uniform<int>(0, 1) == 1;

    bool placed = false;
    for (int attempt = 0; attempt < kMaxPickAttempts && !placed; ++attempt) {
      if (want_heal) {
        const int which = rng.uniform<int>(0, cut.size() - 1);

        auto it = cut.begin();

        for (int s = 0; s < which; ++s) {
          ++it;
        }

        const std::pair<int, int> edge = *it;
        cut.erase(it);

        const TTime t = rng.uniform<TTime>(t_min, t_max);

        link_actions_.push_back(ChaosLinkAction{
            .kind = ChaosLinkAction::Kind::kHeal,
            .i = edge.first,
            .j = edge.second,
            .timestamp = t,
        });

        placed = true;

        continue;
      }

      const int i = rng.uniform<int>(0, num_nodes - 1);
      int j = rng.uniform<int>(0, num_nodes - 2);
      if (j >= i) {
        ++j;
      }

      if (std::max(i, j) >= is_core.size()) {
        continue;
      }

      if (is_core[i] && is_core[j]) {
        continue;
      }

      const std::pair<int, int> edge = NormPair(i, j);
      if (cut.contains(edge)) {
        continue;
      }

      cut.insert(edge);
      const TTime t = rng.uniform<TTime>(t_min, t_max);
      link_actions_.push_back(ChaosLinkAction{
          .kind = ChaosLinkAction::Kind::kPartition,
          .i = edge.first,
          .j = edge.second,
          .timestamp = t,
      });

      placed = true;
    }
  }

  auto LinkActionsComp = [](const ChaosLinkAction& a,
                            const ChaosLinkAction& b) {
    if (a.timestamp != b.timestamp) {
      return a.timestamp < b.timestamp;
    }

    if (a.kind != b.kind) {
      return a.kind < b.kind;
    }

    if (a.i != b.i) {
      return a.i < b.i;
    }

    return a.j < b.j;
  };

  std::sort(link_actions_.begin(), link_actions_.end(), LinkActionsComp);

  std::set<std::pair<int, int>> replay;
  for (const ChaosLinkAction& a : link_actions_) {
    const std::pair<int, int> edge = NormPair(a.i, a.j);

    if (a.kind == ChaosLinkAction::Kind::kPartition) {
      replay.insert(edge);

    } else {
      replay.erase(edge);
    }
  }

  final_cut_ = std::move(replay);
}

inline std::vector<TTime> RandomClientTimes(Random& rng, int count, TTime t_min,
                                            TTime t_max) {
  std::vector<TTime> times;
  times.reserve(count);

  for (int i = 0; i < count; ++i) {
    times.push_back(rng.uniform<TTime>(t_min, t_max));
  }

  std::sort(times.begin(), times.end());

  return times;
}

}  // namespace distsysenv::mc
