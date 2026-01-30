#include "mailbox.h"

namespace distsysenv {

Mailbox::Mailbox(SendFunction send_function)
    : send_function_(std::move(send_function)) {}

void Mailbox::Send(NodeID to_id, Message msg) {
  send_function_(to_id, std::move(msg));
}

}  // namespace distsysenv
