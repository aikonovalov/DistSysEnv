#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include "../node/node.h"
#include "../utils/node_id_manager.h"
#include "../utils/utils.h"
#include "event.h"

namespace distsysenv {

using EventQueue = std::priority_queue<Event, std::vector<Event>, EventEarlier>;

class Logger;

class EventManager {
 public:
  SimulationClock Now() const;

  void Schedule(const Event& event);

  std::optional<Event> PopCurrentEvent();

  void HandleEvent(const Event& curr_event);

  bool Step(const std::function<bool(const Event&)>& exit_functor =
                [](const Event&) { return false; });

  void Process();

  void ProcessUntil(SimulationClock until);

  void SetLogger(Logger&& logger);
  
  void SendLocal(NodeID to, Message msg);

  template <typename T>
  NodeID RegisterNode(T node);

  template <typename T>
  void RegisterNetwork(T node);

 private:
  SimulationClock now_ = 0.0;
  EventQueue queue_;

  NodeIDManager id_manager_;
  std::unordered_map<NodeID, NodeHandler> node_pool_;

  std::unique_ptr<Logger> logger_ = nullptr;
};

template <typename T>
NodeID EventManager::RegisterNode(T node) {
  NodeID id = id_manager_.Generate();

  auto node_owner = std::make_shared<T>(std::move(node));
  NodeHandler handler = [node_owner](const Event& event, Context& ctx) {
    if (event.GetType() == EEventType::kMESSAGE_RECEIVE) {
      const MessageReceivePayload& payload =
          std::get<MessageReceivePayload>(event.GetPayload());
      node_owner->OnMessage(payload.from_id, payload.msg, ctx);

    } else if (event.GetType() == EEventType::kLOCAL_MESSAGE) {
      const LocalMessagePayload& payload =
          std::get<LocalMessagePayload>(event.GetPayload());
      node_owner->OnLocalMessage(payload.msg, ctx);

    } else if (event.GetType() == EEventType::kTIMER) {
      const TimerPayload& payload = std::get<TimerPayload>(event.GetPayload());
      node_owner->OnTimer(payload.timer_name, ctx);
    }
  };

  node_pool_[id] = std::move(handler);
  return id;
}

template <typename T>
void EventManager::RegisterNetwork(T node) {
  NodeID network_id = id_manager_.GetNetworkID();

  auto node_owner = std::make_shared<T>(std::move(node));
  NodeHandler handler = [node_owner](const Event& event, Context& ctx) {
    node_owner->HandleEvent(event, ctx);
  };

  node_pool_[network_id] = std::move(handler);
}

}  // namespace distsysenv
