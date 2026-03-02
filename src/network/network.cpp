#include "network.h"
#include <cassert>
#include <random>
#include "../core/event.h"
#include "../node/context.h"

namespace distsysenv {

Network::Network(const NetworkSettings& settings, uint64_t seed)
    : settings_(settings), rng_(seed) {}

void Network::HandleEvent(const Event& event, Context& ctx) {
  if (event.GetType() == EEventType::kMESSAGE_SEND) {
    const auto& payload = std::get<MessageSendPayload>(event.GetPayload());

    HandleMessageSend(payload.from_id, payload.to_id, payload.msg, ctx);
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
  std::uniform_int_distribution<SimulationClock> delay_distribution(
      settings_.min_delay, settings_.max_delay);

  return delay_distribution(rng_);
}

}  // namespace distsysenv
