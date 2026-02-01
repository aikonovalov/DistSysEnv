#pragma once

#include <functional>
#include <unordered_map>
#include "../core/event.h"

namespace distsysenv {

class Logger {
 public:
  Logger() = default;

  void RegisterHandler(EEventType event_type,
                       std::function<void(const Event&)>&& handler);

  void operator()(const Event& event);

 private:
  std::unordered_map<EEventType, std::function<void(const Event&)>>
      event_handlers_;
};

}  // namespace distsysenv
