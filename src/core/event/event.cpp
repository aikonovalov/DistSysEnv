#include "event.h"

#include <tuple>

namespace distsysenv {

bool EventEarlier::operator()(const Event& a, const Event& b) const {
  return std::tie(a.timestamp, a.to, a.from) >
         std::tie(b.timestamp, b.to, b.from);
}

}  // namespace distsysenv
