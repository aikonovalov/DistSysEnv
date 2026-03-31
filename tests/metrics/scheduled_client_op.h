#pragma once

#include "../../raft/message_specs.h"
#include "../../src/utils/time.h"

namespace distsysenv::metrics {

struct ScheduledClientOp {
  TTime at = 0;
  TCommand command;
};

}  // namespace distsysenv::metrics
