#pragma once

#include "../utils/message.h"
#include "../utils/node_id.h"
#include "../utils/utils.h"

namespace distsysenv {

class EventManager;
class Event;

class Context {
 public:
  Context(EventManager& event_manager, NodeID own_id);

  void Send(NodeID to_id, Message msg);

  void SendLocal(Message msg);

  void SetTimer(const TTimerName& timer_name, SimulationClock duration);

  void ScheduleEvent(const Event& event);

  SimulationClock Now() const;

  NodeID GetOwnID() const;

 private:
  EventManager& event_manager_;
  NodeID id_;
};

}  // namespace distsysenv
