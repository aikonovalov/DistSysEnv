#pragma once

#include "../utils/message.h"
#include "../utils/node_id.h"
#include "../utils/utils.h"

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
  NodeID from_id;
  NodeID to_id;
  Message msg;
};

struct MessageReceivePayload {
  NodeID from_id;
  NodeID to_id;
  Message msg;
};

struct NodeFailPayload {
  NodeID node_id;
};

struct NodeRecoverPayload {
  NodeID node_id;
};

using EventPayload = std::variant<MessageSendPayload, MessageReceivePayload,
                                  NodeFailPayload, NodeRecoverPayload>;

class Event {
  Event(EEventType type, SimulationClock timestamp, EventPayload payload);

 public:
  static Event MessageSend(SimulationClock at, NodeID from_id, NodeID to_id,
                           Message msg);

  static Event MessageReceive(SimulationClock at, NodeID from_id, NodeID to_id,
                              const Message& msg);

  static Event NodeFail(SimulationClock at, NodeID node_id);

  static Event NodeRecover(SimulationClock at, NodeID node_id);

  const EEventType& GetType() const;

  const SimulationClock& GetTimestamp() const;

  const EventPayload& GetPayload() const;

 private:
  EEventType type_;
  SimulationClock timestamp_;
  EventPayload payload_;
};

struct EventEarlier {
  bool operator()(const Event& a, const Event& b) const;
};

}  // namespace distsysenv
