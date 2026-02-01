#include "network.h"

namespace distsysenv {

Network::Network(EventManager& event_manager, NodePool& node_pool,
                 const NetworkSettings& settings, RandomSeed rng_seed)
    : event_manager_(event_manager),
      node_pool_(node_pool),
      settings_(settings),
      rng_seed_(rng_seed),
      rng_(static_cast<std::mt19937::result_type>(rng_seed_)) {
  node_pool_.SubscribeSendRequest(
      [this](NodeID from_id, NodeID to_id, Message msg) {
        OnSendRequest(from_id, to_id, std::move(msg));
      });
}

void Network::OnEvent(const Event& event) {
  if (event.GetType() == EEventType::kMESSAGE_RECEIVE) {
    HandleMessageReceive(event);
  } else if (event.GetType() == EEventType::kTIMER) {
    HandleTimer(event);
  }
}

void Network::HandleMessageReceive(const Event& event) {
  const MessageReceivePayload& payload =
      std::get<MessageReceivePayload>(event.GetPayload());
  node_pool_.Deliver(payload.from_id, payload.to_id, payload.msg);
}

void Network::HandleTimer(const Event& event) {
  const TimerPayload& payload = std::get<TimerPayload>(event.GetPayload());
  node_pool_.DeliverTimer(payload.node_id, payload.timer_name);
}

void Network::OnSendRequest(NodeID from_id, NodeID to_id, Message msg) {
  SimulationClock now = event_manager_.Now();
  Message msg_for_receive = msg;

  if (ShouldDrop(from_id, to_id)) {
    return;
  }

  SimulationClock delay = RandomDelay(from_id, to_id);
  SimulationClock delivery_time = now + delay;
  event_manager_.Schedule(
      Event::MessageReceive(delivery_time, from_id, to_id, msg_for_receive));
}

bool Network::ShouldDrop(NodeID from_id, NodeID to_id) {
  if (settings_.drop_chance <= 0.0f) {
    return false;
  }

  if (settings_.drop_chance >= 1.0f) {
    return true;
  }

  uint64_t h = static_cast<uint64_t>(from_id.GetHash()) ^
               (static_cast<uint64_t>(to_id.GetHash()) << 1);
  std::mt19937 pair_rng(static_cast<std::mt19937::result_type>(h));

  std::bernoulli_distribution drop_distribution(settings_.drop_chance);
  return drop_distribution(pair_rng);
}

SimulationClock Network::RandomDelay(NodeID from_id, NodeID to_id) {
  uint64_t h = static_cast<uint64_t>(from_id.GetHash()) ^
               (static_cast<uint64_t>(to_id.GetHash()) << 1);
  std::mt19937 pair_rng(static_cast<std::mt19937::result_type>(h));

  std::uniform_int_distribution<SimulationClock> delay_distribution(
      settings_.min_delay, settings_.max_delay);
  return delay_distribution(pair_rng);
}

}  // namespace distsysenv
