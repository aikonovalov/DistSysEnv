#pragma once

#include <cstdint>
#include <string>

#include "../../core/event/event.h"
#include "../../core/message/message.h"
#include "../../core/node_id/node_id.h"
#include "../../utils/time.h"
#include "../../utils/utils.h"

namespace distsysenv {

enum class SimulationEventKind : uint8_t {
  Message = 0,
  LocalMessage = 1,
  Timer = 2,
  NodeStatus = 3,
  PartitionPair = 4,
};

enum class MessageDeliveryStatus : uint8_t {
  Sended = 0,
  Received = 1,
  Failed = 2,
};

struct MessageEventPayload {
  MessageDeliveryStatus status;
  NodeID from;
  NodeID to;
  Message msg;
};

struct LocalMessageEventPayload {
  NodeID from;
  Message msg;
};

struct TimerEventPayload {
  NodeID node;
  std::string name;
  uint64_t token = 0;
};

struct NodeStatusEventPayload {
  enum class Status : uint8_t {
    Normal = 0,
    Fail = 1,
  } status;
  NodeID node_id;
};

struct PartitionPairEventPayload {
  NodeID endpoint_a;
  NodeID endpoint_b;
  bool isolate = false;
};

namespace detail {

template <typename T>
struct Creator;

template <>
struct Creator<MessageEventPayload> {
  static Event Of(TTime ts, MessageEventPayload payload);
};

template <>
struct Creator<LocalMessageEventPayload> {
  static Event Of(TTime ts, NodeID to, LocalMessageEventPayload payload);
};

template <>
struct Creator<TimerEventPayload> {
  static Event Of(TTime ts, TimerEventPayload payload);
};

template <>
struct Creator<NodeStatusEventPayload> {
  static Event Of(TTime ts, NodeID from, NodeID to,
                  NodeStatusEventPayload payload);
};

template <>
struct Creator<PartitionPairEventPayload> {
  static Event Of(TTime ts, NodeID from, NodeID to,
                  PartitionPairEventPayload payload);
};

}  // namespace detail

class SimulationEvent {
 public:
  template <class T>
  using make = detail::Creator<T>;
};

Event MakeRoutedApplicationMessage(TTime ts, NodeID transport_to,
                                   MessageEventPayload payload);

struct DecodedMessageEvent {
  MessageDeliveryStatus status;
  NodeID from;
  NodeID to;
  Message msg;
};

Event MakeFailedDeliveryToSenderEvent(TTime ts, NodeID sender,
                                      NodeID intended_to, const Message& msg);

struct DecodedLocalMessageEvent {
  NodeID from;
  Message msg;
};

struct DecodedTimerEvent {
  std::string name;
  uint64_t token = 0;
};

struct DecodedNodeStatusEvent {
  NodeStatusEventPayload::Status status;
  NodeID node_id;
};

struct DecodedPartitionPairEvent {
  NodeID endpoint_a{Index{0}, Generation{0}};
  NodeID endpoint_b{Index{0}, Generation{0}};
  bool isolate = false;
};

Status ClassifySimulationEvent(const Event& event,
                               SimulationEventKind* out_kind);

Status TryDecodeMessageEvent(const Event& event, DecodedMessageEvent* out);
Status TryDecodeLocalMessageEvent(const Event& event,
                                  DecodedLocalMessageEvent* out);
Status TryDecodeTimerEvent(const Event& event, DecodedTimerEvent* out);
Status TryDecodeNodeStatusEvent(const Event& event,
                                DecodedNodeStatusEvent* out);
Status TryDecodePartitionPairEvent(const Event& event,
                                   DecodedPartitionPairEvent* out);

}  // namespace distsysenv
