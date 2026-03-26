#include "event.h"

namespace distsysenv {

bool EventEarlier::operator()(const Event& a, const Event& b) const {
  return a.timestamp > b.timestamp;
}

}  // namespace distsysenv
