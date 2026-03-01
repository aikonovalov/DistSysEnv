#pragma once

#include "../utils/utils.h"

namespace distsysenv {

struct NetworkSettings {
  float drop_prob = 0.0f;
  SimulationClock min_delay = 1;
  SimulationClock max_delay = 10;
};

}  // namespace distsysenv
