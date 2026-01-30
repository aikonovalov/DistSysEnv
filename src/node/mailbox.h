#pragma once

#include <functional>
#include "../utils/message.h"
#include "../utils/node_id.h"

namespace distsysenv {

class Mailbox {
 public:
  using SendFunction = std::function<void(NodeID to_id, Message msg)>;

  explicit Mailbox(SendFunction send_function);

  void Send(NodeID to_id, Message msg);

 private:
  SendFunction send_function_;
};

}  // namespace distsysenv
