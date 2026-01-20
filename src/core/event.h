#pragma once

#include "../utils/utils.h"
#include "../utils/message.h"
#include "../sim/node_id.h"

#include <tuple>
#include <variant>

namespace distsysenv {

enum class EEventType {
    kMESSAGE_SEND,
    kMESSAGE_RECEIVE,
    kNODE_FAIL,
    kNODE_RECOVER
};

struct MessageSendPayload {
    NodeId from;
    NodeId to;
    Message msg;
};

struct MessageReceivePayload {
    NodeId from;
    NodeId to;
    Message msg;
};

struct NodeFailPayload {
    NodeId node;
};

struct NodeRecoverPayload {
    NodeId node;
};

using EventPayload = std::variant<MessageSendPayload, MessageReceivePayload, NodeFailPayload, NodeRecoverPayload>;

struct EventEarlier;

class Event {
    friend EventEarlier;

    Event(EEventType type, SimulationClock timestamp, EventPayload payload);

public:
    static Event MessageSend(SimulationClock at, NodeId from, NodeId to, Message msg);

    static Event MessageReceive(SimulationClock at, NodeId from, NodeId to, const Message& msg);

    static Event NodeFail(SimulationClock at, NodeId node);

    static Event NodeRecover(SimulationClock at, NodeId node);

    SimulationClock GetTimestamp() const;

private:
    EEventType type;
    SimulationClock timestamp;
    EventPayload payload;

};

struct EventEarlier {
    bool operator()(const Event& a, const Event& b) const;
};

}
