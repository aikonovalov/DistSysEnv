#include "event.h"

namespace distsysenv {

Event::Event(EEventType type, NodeID to_node_id, SimulationClock timestamp,
             EventPayload payload)
    : type_(type),
      to_node_id_(to_node_id),
      timestamp_(timestamp),
      payload_(payload) {}

Event Event::MessageSend(SimulationClock at, NodeID from_id, NodeID to_id,
                         Message msg) {
  return Event(EEventType::kMESSAGE_SEND, to_id, at,
               MessageSendPayload{from_id, to_id, std::move(msg)});
}

Event Event::MessageReceive(SimulationClock at, NodeID from_id, NodeID to_id,
                            const Message& msg) {
  return Event(EEventType::kMESSAGE_RECEIVE, to_id, at,
               MessageReceivePayload{from_id, to_id, msg});
}

Event Event::MessageDropped(SimulationClock at, NodeID from_id, NodeID to_id,
                            const Message& msg) {
  return Event(EEventType::kMESSAGE_DROPPED, to_id, at,
               MessageDroppedPayload{from_id, to_id, msg});
}

Event Event::LocalMessage(SimulationClock at, NodeID node_id, Message msg) {
  return Event(EEventType::kLOCAL_MESSAGE, node_id, at,
               LocalMessagePayload{node_id, std::move(msg)});
}

Event Event::CheckerMessage(SimulationClock at, NodeID checker_id,
                            Message msg) {
  return Event(EEventType::kCHECKER_MESSAGE, checker_id, at,
               CheckerMessagePayload{std::move(msg)});
}

Event Event::Timer(SimulationClock at, NodeID node_id,
                   const std::string& timer_name) {
  return Event(EEventType::kTIMER, node_id, at,
               TimerPayload{node_id, timer_name});
}

Event Event::NodeFail(SimulationClock at, NodeID node_id) {
  return Event(EEventType::kNODE_FAIL, node_id, at, NodeFailPayload{node_id});
}

Event Event::NodeRecover(SimulationClock at, NodeID node_id) {
  return Event(EEventType::kNODE_RECOVER, node_id, at,
               NodeRecoverPayload{node_id});
}

const EEventType& Event::GetType() const {
  return type_;
}

const SimulationClock& Event::GetTimestamp() const {
  return timestamp_;
}

const NodeID& Event::GetToNodeId() const {
  return to_node_id_;
}

const EventPayload& Event::GetPayload() const {
  return payload_;
}

bool EventEarlier::operator()(const Event& a, const Event& b) const {
  return std::tie(a.GetTimestamp(), a.GetType()) >
         std::tie(b.GetTimestamp(), b.GetType());
}

}  // namespace distsysenv
