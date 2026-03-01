#pragma once

#include "../utils/node_id.h"
#include "../utils/message.h"
#include <functional>
#include <string>

namespace distsysenv {

class Context;
class Event;

using NodeHandler = std::function<void(const Event& event, Context& ctx)>;

}  // namespace distsysenv
