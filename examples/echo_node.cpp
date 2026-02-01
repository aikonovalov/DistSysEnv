#include "echo_node.h"

#include "../src/utils/message.h"
#include "../src/utils/node_id.h"
#include <string>

namespace distsysenv {

EchoNode::EchoNode(Mailbox mailbox, TimerManager timer_manager)
    : mailbox_(std::move(mailbox)), timer_manager_(std::move(timer_manager)) {
  timer_manager_.SetTimer("GOOOOOOOOOOOOOL", 10);
}

void EchoNode::OnMessage(NodeID from, const Message& msg) {
  std::string type =
      "echo" + std::to_string(static_cast<int32_t>(from.GetIndex()));

  mailbox_.Send(from, Message::FromDescription(type, {}));
}

void EchoNode::OnTimer(const std::string& timer_name) {
  if (timer_name == "GOOOOOOOOOOOOOL") {
    timer_manager_.SetTimer("GOOOOOOOOOOOOOL", 10);
  }
}

}  // namespace distsysenv
