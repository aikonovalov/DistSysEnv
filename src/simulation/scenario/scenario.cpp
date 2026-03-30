#include "scenario.h"

#include <cassert>

namespace distsysenv {

SimulationScenario::SimulationScenario(Network::Config network_config) {
  NodeID id = manager_.AddNode(MakeNetworkHandler(std::move(network_config)));
  simulation_options_.network_gateway = id;
}

EventManager& SimulationScenario::Manager() {
  return manager_;
}

const EventManager& SimulationScenario::Manager() const {
  return manager_;
}

Status SimulationScenario::NetworkGateway(NodeID* gateway_id) const {
  if (gateway_id == nullptr) {
    return Status::ERROR;
  }

  if (!simulation_options_.network_gateway.has_value()) {
    return Status::ERROR;
  }

  *gateway_id = *simulation_options_.network_gateway;
  return Status::OK;
}

void SimulationScenario::SetCheckerSink(NodeID checker_id) {
  simulation_options_.checker_local_sink = checker_id;
}

void SimulationScenario::ScheduleMessage(TTime at, NodeID from, NodeID to,
                                         Message msg) {
  MessageEventPayload payload{MessageDeliveryStatus::Sended, from, to,
                              std::move(msg)};
  if (simulation_options_.network_gateway.has_value()) {
    manager_.PushEvent(MakeRoutedApplicationMessage(
        at, *simulation_options_.network_gateway, std::move(payload)));
  }
}

void SimulationScenario::ScheduleLocalMessage(TTime at, NodeID to,
                                              NodeID logical_from,
                                              Message msg) {
  manager_.PushEvent(SimulationEvent::make<LocalMessageEventPayload>::Of(
      at, to, LocalMessageEventPayload{logical_from, std::move(msg)}));
}

void SimulationScenario::ScheduleNodeFail(TTime at, NodeID target,
                                          NodeID control_from) {
  assert(simulation_options_.network_gateway.has_value() &&
         "ScheduleNodeFail requires a network node");

  NodeStatusEventPayload payload{.status = NodeStatusEventPayload::Status::Fail,
                                 .node_id = target};
  manager_.PushEvent(SimulationEvent::make<NodeStatusEventPayload>::Of(
      at, control_from, *simulation_options_.network_gateway,
      std::move(payload)));
}

void SimulationScenario::ScheduleNodeRecover(TTime at, NodeID target,
                                             NodeID control_from) {
  assert(simulation_options_.network_gateway.has_value() &&
         "ScheduleNodeRecover requires a network node");

  NodeStatusEventPayload payload{
      .status = NodeStatusEventPayload::Status::Normal, .node_id = target};

  manager_.PushEvent(SimulationEvent::make<NodeStatusEventPayload>::Of(
      at, control_from, *simulation_options_.network_gateway,
      std::move(payload)));
}

void SimulationScenario::SchedulePartitionPair(TTime at, NodeID endpoint_a,
                                               NodeID endpoint_b, bool isolate,
                                               NodeID control_from) {
  assert(simulation_options_.network_gateway.has_value() &&
         "SchedulePartitionPair requires a network node");

  PartitionPairEventPayload payload{
      .endpoint_a = endpoint_a, .endpoint_b = endpoint_b, .isolate = isolate};
  manager_.PushEvent(SimulationEvent::make<PartitionPairEventPayload>::Of(
      at, control_from, *simulation_options_.network_gateway,
      std::move(payload)));
}

void SimulationScenario::RunUntil(TTime until) {
  manager_.ProcessUntil(until);
}

}  // namespace distsysenv
