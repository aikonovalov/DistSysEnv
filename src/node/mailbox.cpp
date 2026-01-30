#include "mailbox.h"

namespace distsysenv {

Mailbox::Mailbox(SendFunction send_function)
    : send_function_(std::move(send_function)) {}

void Mailbox::Send(NodeID to, Message msg) {
  send_function_(to, std::move(msg));
}

}  // namespace distsysenv
