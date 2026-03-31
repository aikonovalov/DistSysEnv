#include "event.h"

#include <tuple>

namespace distsysenv {

bool EventEarlier::operator()(const Event& a, const Event& b) const {
  return std::tie(a.timestamp, a.to, a.dispatch_order, a.from) >
         std::tie(b.timestamp, b.to, b.dispatch_order, b.from);
}

}  // namespace distsysenv
