#include "node_pool.h"
#include "../core/event.h"
#include "../core/event_manager.h"

namespace distsysenv {

NodePool::NodePool(EventManager& event_manager)
    : event_manager_(&event_manager) {}

NodeID NodePool::CreateNode(NodeFactory factory) {
  NodeID new_id = id_manager_.Generate();

  Mailbox mailbox([this, new_id](NodeID to_id, Message msg) {
    RequestSend(new_id, to_id, std::move(msg));
  });

  TimerManager timer_manager([this, new_id](const std::string& timer_name,
                                            SimulationClock time_amount) {
    if (!event_manager_)
      return;
    SimulationClock at = event_manager_->Now() + time_amount;
    event_manager_->Schedule(Event::Timer(at, new_id, timer_name));
  });

  NodeFactoryResult result =
      factory(std::move(mailbox), std::move(timer_manager));
  nodes_[new_id.GetIndex()] = std::move(result);

  return new_id;
}

void NodePool::Deliver(NodeID from_id, NodeID to_id, const Message& msg) {
  auto it = nodes_.find(to_id.GetIndex());

  if (it == nodes_.end() || !id_manager_.IsValid(to_id)) {
    return;
  }

  it->second.first(from_id, msg);
}

void NodePool::DeliverTimer(NodeID node_id, const std::string& timer_name) {
  auto it = nodes_.find(node_id.GetIndex());
  if (it == nodes_.end() || !id_manager_.IsValid(node_id))
    return;
  it->second.second(timer_name);
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
