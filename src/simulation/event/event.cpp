#include "event.h"

#include "../../utils/utils.h"

namespace distsysenv {

namespace {

Bytes EncodeMessagePayload(const MessageEventPayload& p) {
  return BuildPayload(SimulationEventKind::Message, p.status, p.from, p.to,
                      p.msg.Serialize());
}

Bytes EncodeLocalMessagePayload(const LocalMessageEventPayload& p) {
  return BuildPayload(SimulationEventKind::LocalMessage, p.from,
                      p.msg.Serialize());
}

Bytes EncodeTimerPayload(const TimerEventPayload& p) {
  return BuildPayload(SimulationEventKind::Timer, p.token, p.name);
}

Bytes EncodeNodeStatusPayload(const NodeStatusEventPayload& p) {
  return BuildPayload(SimulationEventKind::NodeStatus, p.status, p.node_id);
}

Bytes EncodePartitionPairPayload(const PartitionPairEventPayload& p) {
  const uint8_t cut = p.isolate;

  return BuildPayload(SimulationEventKind::PartitionPair, p.endpoint_a,
                      p.endpoint_b, cut);
}

bool IsKnownMessageDeliveryStatus(MessageDeliveryStatus s) {
  return s == MessageDeliveryStatus::Sended ||
         s == MessageDeliveryStatus::Received ||
         s == MessageDeliveryStatus::Failed;
}

bool IsKnownNodeStatusPayloadStatus(NodeStatusEventPayload::Status s) {
  return s == NodeStatusEventPayload::Status::Normal ||
         s == NodeStatusEventPayload::Status::Fail;
}

}  // namespace

namespace detail {

Event Creator<MessageEventPayload>::Of(TTime ts, MessageEventPayload payload) {
  return Event{payload.from, payload.to, ts, EncodeMessagePayload(payload), 0};
}

Event Creator<LocalMessageEventPayload>::Of(TTime ts, NodeID to,
                                            LocalMessageEventPayload payload) {
  return Event{payload.from, to, ts, EncodeLocalMessagePayload(payload), 0};
}

Event Creator<TimerEventPayload>::Of(TTime ts, TimerEventPayload payload) {
  return Event{payload.node, payload.node, ts, EncodeTimerPayload(payload), 1};
}

Event Creator<NodeStatusEventPayload>::Of(TTime ts, NodeID from, NodeID to,
                                          NodeStatusEventPayload payload) {
  return Event{from, to, ts, EncodeNodeStatusPayload(payload), 0};
}

Event Creator<PartitionPairEventPayload>::Of(
    TTime ts, NodeID from, NodeID to, PartitionPairEventPayload payload) {
  return Event{from, to, ts, EncodePartitionPairPayload(payload), 0};
}

}  // namespace detail

Event MakeRoutedApplicationMessage(TTime ts, NodeID transport_to,
                                   MessageEventPayload payload) {
  return Event{payload.from, transport_to, ts, EncodeMessagePayload(payload),
               0};
}

Event MakeFailedDeliveryToSenderEvent(TTime ts, NodeID sender,
                                      NodeID intended_to, const Message& msg) {
  MessageEventPayload payload{MessageDeliveryStatus::Failed, sender,
                              intended_to, msg};
  return Event{sender, sender, ts, EncodeMessagePayload(payload), 0};
}

Status ClassifySimulationEvent(const Event& event,
                               SimulationEventKind* out_kind) {
  if (out_kind == nullptr) {
    return Status::ERROR;
  }

  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  TOffset off = 0;

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::Message &&
      kind != SimulationEventKind::LocalMessage &&
      kind != SimulationEventKind::Timer &&
      kind != SimulationEventKind::NodeStatus &&
      kind != SimulationEventKind::PartitionPair) {
    return Status::ERROR;
  }

  *out_kind = kind;
  return Status::OK;
}

Status TryDecodeMessageEvent(const Event& event, DecodedMessageEvent* out) {
  if (out == nullptr) {
    return Status::ERROR;
  }

  TOffset off = 0;
  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::Message) {
    return Status::ERROR;
  }

  MessageDeliveryStatus status{};
  read_field(event.data, off, status);

  if (!IsKnownMessageDeliveryStatus(status)) {
    return Status::ERROR;
  }

  read_field(event.data, off, out->from);
  read_field(event.data, off, out->to);

  Bytes wire;
  read_field(event.data, off, wire);
  out->msg = Message::Deserialize(wire);

  if (off != event.data.size()) {
    return Status::ERROR;
  }

  out->status = status;

  return Status::OK;
}

Status TryDecodeLocalMessageEvent(const Event& event,
                                  DecodedLocalMessageEvent* out) {
  if (out == nullptr) {
    return Status::ERROR;
  }

  TOffset off = 0;
  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::LocalMessage) {
    return Status::ERROR;
  }

  read_field(event.data, off, out->from);

  Bytes wire;
  read_field(event.data, off, wire);
  out->msg = Message::Deserialize(wire);

  if (off != event.data.size()) {
    return Status::ERROR;
  }

  return Status::OK;
}

Status TryDecodeTimerEvent(const Event& event, DecodedTimerEvent* out) {
  if (out == nullptr) {
    return Status::ERROR;
  }

  TOffset off = 0;
  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::Timer) {
    return Status::ERROR;
  }

  read_field(event.data, off, out->token);
  read_field(event.data, off, out->name);
  if (off != event.data.size()) {
    return Status::ERROR;
  }

  return Status::OK;
}

Status TryDecodeNodeStatusEvent(const Event& event,
                                DecodedNodeStatusEvent* out) {
  if (out == nullptr) {
    return Status::ERROR;
  }

  TOffset off = 0;
  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::NodeStatus) {
    return Status::ERROR;
  }

  NodeStatusEventPayload::Status status{};
  read_field(event.data, off, status);
  if (!IsKnownNodeStatusPayloadStatus(status)) {
    return Status::ERROR;
  }

  read_field(event.data, off, out->node_id);

  if (off != event.data.size()) {
    return Status::ERROR;
  }

  out->status = status;

  return Status::OK;
}

Status TryDecodePartitionPairEvent(const Event& event,
                                   DecodedPartitionPairEvent* out) {
  if (out == nullptr) {
    return Status::ERROR;
  }

  TOffset off = 0;
  if (event.data.size() < sizeof(SimulationEventKind)) {
    return Status::ERROR;
  }

  SimulationEventKind kind{};
  read_field(event.data, off, kind);
  if (kind != SimulationEventKind::PartitionPair) {
    return Status::ERROR;
  }

  read_field(event.data, off, out->endpoint_a);
  read_field(event.data, off, out->endpoint_b);

  uint8_t cut{};
  read_field(event.data, off, cut);

  if (cut > 1) {
    return Status::ERROR;
  }

  if (off != event.data.size()) {
    return Status::ERROR;
  }

  out->isolate = cut != 0;

  return Status::OK;
}

}  // namespace distsysenv
