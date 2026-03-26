#pragma once

#include "../../utils/time.h"
#include "../../utils/utils.h"
#include "../node_id/node_id.h"

#include <any>
#include <string>
#include <tuple>
#include <variant>

namespace distsysenv {

struct Event {
  NodeID from;
  NodeID to;
  TTime timestamp;
  Bytes data;
};

struct EventEarlier {
  bool operator()(const Event& a, const Event& b) const;
};

}  // namespace distsysenv
