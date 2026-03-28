#include "network_node.h"

#include <algorithm>

#include "../../core/context/context.h"

namespace distsysenv {

Network::Network(Config config)
    : settings_(std::move(config.behavior)), rng_(config.random_seed) {}

void Network::OnEvent(const Event& event, CoreContext& ctx) {
  SimulationEventKind kind{};
  if (ClassifySimulationEvent(event, &kind) != Status::OK) {
    return;
  }

  if (kind == SimulationEventKind::NodeStatus) {
    DecodedNodeStatusEvent dec{
        NodeStatusEventPayload::Status::Normal,
        NodeID(Index{0}, Generation{0}),
    };

    if (TryDecodeNodeStatusEvent(event, &dec) != Status::OK) {
      return;
    }

    SetNodeStatus(dec.status, dec.node_id);

    return;
  }

  if (kind != SimulationEventKind::Message) {
    return;
  }

  DecodedMessageEvent dec{
      MessageDeliveryStatus::Sended,
      NodeID(Index{0}, Generation{0}),
      NodeID(Index{0}, Generation{0}),
      Message::FromDescription("_", {}),
  };

  if (TryDecodeMessageEvent(event, &dec) != Status::OK) {
    return;
  }

  if (dec.status != MessageDeliveryStatus::Sended) {
    return;
  }

  HandleSend(dec.from, dec.to, dec.msg, ctx);
}

void Network::HandleSend(NodeID from, NodeID to, const Message& msg,
                         CoreContext& ctx) {
  if (ShouldDrop(from, to)) {
    ctx.PushEvent(MakeFailedDeliveryToSenderEvent(ctx.Now(), from, to, msg));
    return;
  }

  const TTime delay = std::max(RandomDelay(), TTime{0});
  const TTime arrive = ctx.Now() + delay;
  MessageEventPayload payload{MessageDeliveryStatus::Received, from, to, msg};
  ctx.PushEvent(SimulationEvent::make<MessageEventPayload>::Of(
      arrive, std::move(payload)));
}

void Network::SetNodeStatus(NodeStatusEventPayload::Status status,
                            NodeID node_id) {
  if (status == NodeStatusEventPayload::Status::Fail) {
    node_settings_[node_id].is_failed = NodeNetworkSettings::Status::FAIL;

  } else {
    node_settings_[node_id].is_failed = NodeNetworkSettings::Status::OK;
  }
}

bool Network::ShouldDrop(NodeID from, NodeID to) {
  NodeNetworkSettings& curr_settings = node_settings_[to];

  if (curr_settings.is_failed == NodeNetworkSettings::Status::FAIL ||
      curr_settings.partitioned_from.contains(from)) {
    return true;
  }

  if (settings_.drop_prob <= 0.0f) {
    return false;
  }

  if (settings_.drop_prob >= 1.0f) {
    return true;
  }

  return rng_.uniform<float>(0.0f, 1.0f) < settings_.drop_prob;
}

TTime Network::RandomDelay() {
  return rng_.uniform<TTime>(settings_.min_delay, settings_.max_delay);
}

EventHandler MakeNetworkHandler(Network::Config config) {
  auto net = std::make_shared<Network>(std::move(config));

  return [net](const Event& event, CoreContext& ctx) {
    net->OnEvent(event, ctx);
  };
}

}  // namespace distsysenv
