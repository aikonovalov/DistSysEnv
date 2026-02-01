#pragma once

#include <functional>
#include <unordered_map>
#include "../core/event.h"

namespace distsysenv {

using EventHandler = std::function<void(const Event&)>;
using HandlerMap = std::unordered_map<EEventType, EventHandler>;

const HandlerMap& DefaultHandlers();

class Logger {
 public:
  static Logger Empty();
  static Logger WithDefaultHandlers();

  void RegisterHandler(EEventType event_type, EventHandler&& handler);

  void operator()(const Event& event);

 private:
  Logger() = default;

  std::unordered_map<EEventType, EventHandler> event_handlers_;
};

}  // namespace distsysenv
