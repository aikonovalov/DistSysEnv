#include "raft.h"

namespace distsysenv {

TIndex RaftNode::GetLastLogTerm() {
  if (log_.empty()) {
    return -1;
  }

  const LogEntry& last_log_entry = log_.back();

  return last_log_entry.term;
}

TIndex RaftNode::GetLastLogIndex() {
  if (log_.empty()) {
    return -1;
  }

  return log_.size() - 1;
}

void RaftNode::ApplyCommand(TCommand command) {
  switch (command.type()) {
    case TCommand::Type::eGET:
      break;
    case TCommand::Type::eSET:
      if (command.value().has_value()) {
        kv_store_.Set(command.key(), *command.value());
      }

      break;
    case TCommand::Type::eDEL:
      kv_store_.Delete(command.key());

      break;
  }
}

void RaftNode::ApplyCommittedEntries() {
  while (last_applied_ < commit_index_) {
    ++last_applied_;

    TCommand curr_command = log_[last_applied_].command;

    ApplyCommand(curr_command);
  }
}

}  // namespace distsysenv
