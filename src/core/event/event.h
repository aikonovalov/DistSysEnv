#pragma once

#include "../../utils/time.h"
#include "../../utils/utils.h"
#include "../node_id/node_id.h"

#include <any>
#include <cstdint>
#include <string>
#include <tuple>
#include <variant>

namespace distsysenv {

struct Event {
  NodeID from;
  NodeID to;
  TTime timestamp;
  Bytes data;
  uint8_t dispatch_order = 0;
};

struct EventEarlier {
  bool operator()(const Event& a, const Event& b) const;
};

}  // namespace distsysenv
