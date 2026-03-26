#pragma once

#include <functional>
#include "event.h"

namespace distsysenv {

class CoreContext;

using EventHandler = std::function<void(const Event& event, CoreContext& ctx)>;

}  // namespace distsysenv
