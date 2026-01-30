#include "node_pool.h"

namespace distsysenv {

NodeID NodePool::CreateNode(NodeFactory factory) {
  NodeID new_id = id_manager_.Generate();

  Mailbox mailbox([this, new_id](NodeID to_id, Message msg) {
    RequestSend(new_id, to_id, std::move(msg));
  });

  DeliveryFunction deliver = factory(std::move(mailbox));
  nodes_[new_id.GetIndex()] = std::move(deliver);

  return new_id;
}

void NodePool::Deliver(NodeID from_id, NodeID to_id, const Message& msg) {
  auto it = nodes_.find(to_id.GetIndex());

  if (it == nodes_.end() || !id_manager_.IsValid(to_id)) {
    return;
  }

  it->second(from_id, msg);
}

void NodePool::SubscribeSendRequest(SendRequestObserver observer) {
  send_request_observer_ = std::move(observer);
}

bool NodePool::HasNode(NodeID id) const {
  return id_manager_.IsValid(id) && nodes_.contains(id.GetIndex());
}

void NodePool::RequestSend(NodeID from_id, NodeID to_id, Message msg) {
  if (send_request_observer_) {
    send_request_observer_(from_id, to_id, std::move(msg));
  }
}

}  // namespace distsysenv
