#include "network.h"
#include <cassert>
#include <random>
#include "../core/event.h"
#include "../node/context.h"
#include "network_settings.h"

namespace distsysenv {

Network::Network(const NetworkSettings& settings, uint64_t seed)
    : settings_(settings), rng_(seed) {}

void Network::HandleEvent(const Event& event, Context& ctx) {
  if (event.GetType() == EEventType::kMESSAGE_SEND) {
    const auto& payload = std::get<MessageSendPayload>(event.GetPayload());

    HandleMessageSend(payload.from_id, payload.to_id, payload.msg, ctx);

  } else if (event.GetType() == EEventType::kNODE_FAIL) {
    const auto& payload = std::get<NodeFailPayload>(event.GetPayload());

    HandleNodeFail(payload.node_id);

  } else if (event.GetType() == EEventType::kNODE_RECOVER) {
    const auto& payload = std::get<NodeRecoverPayload>(event.GetPayload());

    HandleNodeRecover(payload.node_id);
  }
}

void Network::HandleMessageSend(NodeID from, NodeID to, const Message& msg,
                                Context& ctx) {
  if (ShouldDrop(from, to)) {
    ctx.ScheduleEvent(Event::MessageDropped(ctx.Now(), from, to, msg));

    return;
  }

  SimulationClock delay = RandomDelay();

  assert(delay > 0 && "Delay must be positive");

  ctx.ScheduleEvent(Event::MessageReceive(ctx.Now() + delay, from, to, msg));
}

bool Network::ShouldDrop(NodeID from, NodeID to) {
  NodeNetworkSettings& curr_settings = node_settings_[to];

  if (curr_settings.is_failed ||
      curr_settings.partitioned_from.contains(from)) {
    return true;
  }

  if (settings_.drop_prob <= 0.0f) {
    return false;
  }

  if (settings_.drop_prob >= 1.0f) {
    return true;
  }

  std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  return dist(rng_) < settings_.drop_prob;
}

SimulationClock Network::RandomDelay() {
  std::uniform_real_distribution<SimulationClock> delay_distribution(
      settings_.min_delay, settings_.max_delay);

  return delay_distribution(rng_);
}

void Network::HandleNodeFail(NodeID node_id) {
  if (node_id.GetIndex() < NodeID::Index{0}) {
    return;
  }

  node_settings_[node_id].is_failed = true;
}

void Network::HandleNodeRecover(NodeID node_id) {
  if (node_id.GetIndex() < NodeID::Index{0}) {
    return;
  }

  node_settings_[node_id].is_failed = false;
}

}  // namespace distsysenv
