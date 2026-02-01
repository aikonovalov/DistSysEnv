#pragma once

#include "../src/node/mailbox.h"
#include "../src/node/timer_manager.h"
#include "../src/utils/message.h"
#include "../src/utils/node_id.h"
#include <optional>

namespace distsysenv {

class EchoNode {
 public:
  EchoNode(Mailbox mailbox, TimerManager timer_manager);

  void OnMessage(NodeID from, const Message& msg);
  
  void OnTimer(const std::string& timer_name);

 private:
  Mailbox mailbox_;
  TimerManager timer_manager_;

};

}  // namespace distsysenv
