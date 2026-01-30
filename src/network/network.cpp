#include "network.h"

namespace distsysenv {

Network::Network(EventManager& event_manager, NodePool& node_pool,
                 const NetworkSettings& settings, RandomSeed rng_seed)
    : event_manager_(event_manager),
      node_pool_(node_pool),
      settings_(settings),
      rng_seed_(rng_seed),
      rng_(static_cast<std::mt19937::result_type>(rng_seed_)) {
  node_pool_.SubscribeSendRequest([this](NodeID from, NodeID to, Message msg) {
    OnSendRequest(from, to, std::move(msg));
  });
}

void Network::OnEvent(const Event& event) {
  if (event.GetType() == EEventType::kMESSAGE_RECEIVE) {
    HandleMessageReceive(event);
  }
}

void Network::HandleMessageReceive(const Event& event) {
  const MessageReceivePayload& payload =
      std::get<MessageReceivePayload>(event.GetPayload());
  node_pool_.Deliver(payload.from, payload.to, payload.msg);
}

void Network::OnSendRequest(NodeID from, NodeID to, Message msg) {
  SimulationClock now = event_manager_.Now();
  Message msg_for_receive = msg;
  event_manager_.Schedule(Event::MessageSend(now, from, to, std::move(msg)));

  if (ShouldDrop(from, to)) {
    return;
  }

  SimulationClock delay = RandomDelay(from, to);
  SimulationClock delivery_time = now + delay;
  event_manager_.Schedule(
      Event::MessageReceive(delivery_time, from, to, msg_for_receive));
}

bool Network::ShouldDrop(NodeID from, NodeID to) {
  if (settings_.drop_chance <= 0.f) {
    return false;
  }

  if (settings_.drop_chance >= 1.f) {
    return true;
  }

  uint64_t h = static_cast<uint64_t>(from.GetHash()) ^
               (static_cast<uint64_t>(to.GetHash()) << 1);
  std::mt19937 pair_rng(static_cast<std::mt19937::result_type>(h));

  std::bernoulli_distribution drop_distribution(settings_.drop_chance);
  return drop_distribution(pair_rng);
}

SimulationClock Network::RandomDelay(NodeID from, NodeID to) {
  uint64_t h = static_cast<uint64_t>(from.GetHash()) ^
               (static_cast<uint64_t>(to.GetHash()) << 1);
  std::mt19937 pair_rng(static_cast<std::mt19937::result_type>(h));

  std::uniform_int_distribution<SimulationClock> delay_distribution(
      settings_.min_delay, settings_.max_delay);
  return delay_distribution(pair_rng);
}

}  // namespace distsysenv
