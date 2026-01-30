#pragma once

#include <functional>
#include <unordered_map>
#include "../utils/message.h"
#include "../utils/node_id.h"
#include "../utils/node_id_manager.h"
#include "../utils/utils.h"
#include "mailbox.h"

namespace distsysenv {

using DeliveryFunction = std::function<void(NodeID from_id, const Message&)>;

using NodeFactory = std::function<DeliveryFunction(Mailbox mailbox)>;

using SendRequestObserver =
    std::function<void(NodeID from_id, NodeID to_id, Message msg)>;

class NodePool {
 public:
  NodePool() = default;

  NodeID CreateNode(NodeFactory factory);

  void Deliver(NodeID from_id, NodeID to_id, const Message& msg);

  void SubscribeSendRequest(SendRequestObserver observer);

  bool HasNode(NodeID id) const;

 private:
  void RequestSend(NodeID from_id, NodeID to_id, Message msg);

  NodeIDManager id_manager_;
  std::unordered_map<NodeID::Index, DeliveryFunction> nodes_;
  SendRequestObserver send_request_observer_;
};

}  // namespace distsysenv
