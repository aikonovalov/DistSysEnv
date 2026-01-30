#pragma once

#include "../utils/utils.h"

namespace distsysenv {

struct NetworkSettings {
  float drop_chance = 1.0f;
  SimulationClock min_delay = 1;
  SimulationClock max_delay = 10;
};

}  // namespace distsysenv
