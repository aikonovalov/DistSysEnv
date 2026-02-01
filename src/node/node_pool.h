#pragma once

#include <functional>
#include <unordered_map>
#include "../utils/message.h"
#include "../utils/node_id.h"
#include "../utils/node_id_manager.h"
#include "../utils/utils.h"
#include "mailbox.h"
#include "timer_manager.h"

namespace distsysenv {

class EventManager;

using DeliveryFunction = std::function<void(NodeID from_id, const Message&)>;
using TimerDeliveryFunction = std::function<void(const std::string&)>;

using NodeFactoryResult = std::pair<DeliveryFunction, TimerDeliveryFunction>;
using NodeFactory = std::function<NodeFactoryResult(
    Mailbox mailbox, TimerManager timer_manager)>;

using SendRequestObserver =
    std::function<void(NodeID from_id, NodeID to_id, Message msg)>;

class NodePool {
 public:
  explicit NodePool(EventManager& event_manager);

  NodeID CreateNode(NodeFactory factory);

  void Deliver(NodeID from_id, NodeID to_id, const Message& msg);
  void DeliverTimer(NodeID node_id, const std::string& timer_name);

  void SubscribeSendRequest(SendRequestObserver observer);

  bool HasNode(NodeID id) const;

 private:
  void RequestSend(NodeID from_id, NodeID to_id, Message msg);

  EventManager* event_manager_ = nullptr;
  NodeIDManager id_manager_;
  std::unordered_map<NodeID::Index, NodeFactoryResult> nodes_;
  SendRequestObserver send_request_observer_;
};

}  // namespace distsysenv
