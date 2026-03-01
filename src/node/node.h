#pragma once

#include <functional>
#include <string>
#include "../utils/message.h"
#include "../utils/node_id.h"

namespace distsysenv {

class Context;
class Event;

using NodeHandler = std::function<void(const Event& event, Context& ctx)>;

}  // namespace distsysenv
