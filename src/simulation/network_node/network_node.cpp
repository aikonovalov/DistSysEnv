#include "network_node.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

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

  if (kind == SimulationEventKind::PartitionPair) {
    DecodedPartitionPairEvent dec;

    if (TryDecodePartitionPairEvent(event, &dec) != Status::OK) {
      return;
    }

    ApplyPartitionPair(dec.endpoint_a, dec.endpoint_b, dec.isolate);

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
  TTime arrive = ctx.Now() + delay;

  const std::pair<NodeID, NodeID> link = std::make_pair(from, to);

  const auto it = last_arrival_by_link_.find(link);
  if (it != last_arrival_by_link_.end() && arrive <= it->second) {
    arrive = std::nextafter(it->second, std::numeric_limits<TTime>::infinity());
  }

  last_arrival_by_link_[link] = arrive;

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

void Network::ApplyPartitionPair(NodeID a, NodeID b, bool isolate) {
  if (a == b) {
    return;
  }

  if (isolate) {
    node_settings_[a].partitioned_from.insert(b);
    node_settings_[b].partitioned_from.insert(a);
  } else {
    node_settings_[a].partitioned_from.erase(b);
    node_settings_[b].partitioned_from.erase(a);
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
