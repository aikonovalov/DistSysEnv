#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include "../../utils/utils.h"
#include "../node_id/node_id_manager.h"
#include "event.h"
#include "event_handler.h"
#include "event_queue.h"

namespace distsysenv {

class EventManager {
 public:
  TTime Now() const;

  void PushEvent(const Event& event);
  void HandleEvent(const Event& event);

  Status Step(const std::function<bool(const Event&)>& exit_functor =
                  [](const Event&) { return false; });

  void Process();
  void ProcessUntil(TTime until);

  NodeID RegisterNode(EventHandler handler);

 private:
  TTime now_ = 0.0;
  EventQueue queue_;

  NodeIDManager id_manager_;
  std::unordered_map<NodeID, EventHandler> node_pool_;
};

template <typename T>
EventHandler MakeNodeHandler(T node) {
  auto node_owner = std::make_shared<T>(std::move(node));

  return [node_owner](const Event& event, CoreContext& ctx) {
    node_owner->OnEvent(event, ctx);
  };
}

}  // namespace distsysenv
