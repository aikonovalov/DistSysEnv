#pragma once

#include <memory>
#include <vector>

namespace distsysenv {

using Byte = std::byte;
using Bytes = std::vector<Byte>;

using TOffset = int64_t;
using TTimerName = std::string;
using TIndex = int64_t;
using TCommand = std::string;

using SimulationClock = float;

}  // namespace distsysenv
