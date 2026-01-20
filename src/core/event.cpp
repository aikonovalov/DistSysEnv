#include "event.h"

namespace distsysenv {

Event::Event(EEventType type, SimulationClock timestamp, EventPayload payload) : type(type), timestamp(timestamp), payload(payload) {}

Event Event::MessageSend(SimulationClock at, NodeId from, NodeId to, Message msg) {
    return Event(EEventType::kMESSAGE_SEND, at, MessageSendPayload{from, to, std::move(msg)});
}

Event Event::MessageReceive(SimulationClock at, NodeId from, NodeId to, const Message& msg) {
    return Event(EEventType::kMESSAGE_RECEIVE, at, MessageReceivePayload{from, to, msg});
}

Event Event::NodeFail(SimulationClock at, NodeId node) {
    return Event(EEventType::kNODE_FAIL, at, NodeFailPayload{node});
}

Event Event::NodeRecover(SimulationClock at, NodeId node) {
    return Event(EEventType::kNODE_RECOVER, at, NodeRecoverPayload{node});
}

SimulationClock Event::GetTimestamp() const {
    return timestamp;
}

bool EventEarlier::operator()(const Event& a, const Event& b) const {
    return std::tie(a.timestamp, a.type) > std::tie(b.timestamp, b.type);
}

}