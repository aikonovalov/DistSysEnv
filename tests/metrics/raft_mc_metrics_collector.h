#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "../../raft/message_specs.h"
#include "../../src/core/message/message.h"
#include "../../src/core/node_id/node_id.h"
#include "../../src/simulation/context/context.h"
#include "../../src/simulation/event/event.h"
#include "../../src/simulation/scenario/scenario.h"
#include "../../src/utils/time.h"
#include "scheduled_client_op.h"

namespace distsysenv::metrics {

struct RaftMcMetricsReport {
  uint64_t append_entries_rejects = 0;

  std::vector<TTime> set_latencies;
  std::vector<TTime> get_latencies;

  std::optional<double> MeanSetLatency() const {
    if (set_latencies.empty()) {
      return std::nullopt;
    }

    double sum = 0;
    for (TTime d : set_latencies) {
      sum += static_cast<double>(d);
    }

    return sum / static_cast<double>(set_latencies.size());
  }

  std::optional<double> MeanGetLatency() const {
    if (get_latencies.empty()) {
      return std::nullopt;
    }

    double sum = 0;
    for (TTime latency : get_latencies) {
      sum += static_cast<double>(latency);
    }

    return sum / static_cast<double>(get_latencies.size());
  }
};

class RaftMcMetricsCollector {
 public:
  explicit RaftMcMetricsCollector(
      std::shared_ptr<std::vector<ScheduledClientOp>> schedule,
      std::shared_ptr<std::vector<command::ResponsePayload>> response_log =
          nullptr)
      : state_(std::make_shared<State>()) {
    state_->schedule = std::move(schedule);
    state_->response_log = std::move(response_log);
  }

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    if (state_->schedule && state_->schedule->size() > state_->matched.size()) {
      state_->matched.resize(state_->schedule->size(), false);
    }

    DecodedLocalMessageEvent dec_event{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec_event) != Status::OK) {
      return;
    }

    if (dec_event.msg.GetType() == "raft_metric_append_entries_reject") {
      ++state_->report.append_entries_rejects;
      return;
    }

    if (dec_event.msg.GetType() != "client_command_response") {
      return;
    }

    const command::ResponsePayload resp =
        command::ResponsePayload::Deserialize(dec_event.msg.GetPayload());

    if (state_->response_log) {
      state_->response_log->push_back(resp);
    }

    if (resp.status != Status::OK) {
      return;
    }

    if (!state_->schedule) {
      return;
    }

    const TTime at = e.timestamp;

    if (resp.command.type() == TCommand::Type::eSET) {
      for (size_t i = 0; i < state_->schedule->size(); ++i) {
        if (state_->matched[i]) {
          continue;
        }

        const ScheduledClientOp& op = (*state_->schedule)[i];
        if (op.command.type() != TCommand::Type::eSET) {
          continue;
        }

        if (op.command.key() != resp.command.key()) {
          continue;
        }

        if (op.command.value() != resp.command.value()) {
          continue;
        }

        state_->matched[i] = true;
        state_->report.set_latencies.push_back(at - op.at);
        return;
      }

      return;
    }

    if (resp.command.type() == TCommand::Type::eGET) {
      for (size_t i = 0; i < state_->schedule->size(); ++i) {
        if (state_->matched[i]) {
          continue;
        }

        const ScheduledClientOp& op = (*state_->schedule)[i];
        if (op.command.type() != TCommand::Type::eGET) {
          continue;
        }

        if (op.command.key() != resp.command.key()) {
          continue;
        }

        state_->matched[i] = true;
        state_->report.get_latencies.push_back(at - op.at);
        return;
      }
    }
  }

  RaftMcMetricsReport SnapshotReport() const { return state_->report; }

 private:
  struct State {
    std::shared_ptr<std::vector<ScheduledClientOp>> schedule;
    std::shared_ptr<std::vector<command::ResponsePayload>> response_log;
    std::vector<bool> matched;
    RaftMcMetricsReport report;
  };

  std::shared_ptr<State> state_;
};

template <typename TRaftNode>
inline SimulationScenario BuildRaftMcSimulation(
    const Network::Config& net, const RaftMcMetricsCollector& metrics,
    int num_nodes, std::vector<NodeID>* raft_ids) {
  SimulationScenario sim(net);
  sim.AddNode(metrics, NodeTag::kCHECKER);

  raft_ids->clear();
  raft_ids->reserve(num_nodes);

  for (int i = 0; i < num_nodes; ++i) {
    raft_ids->push_back(sim.AddNode(TRaftNode{{}}));
  }

  return sim;
}

}  // namespace distsysenv::metrics
