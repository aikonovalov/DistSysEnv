#include "logger.h"

#include <iostream>

namespace distsysenv {

const HandlerMap& DefaultHandlers() {
  static const HandlerMap kDefaultHandlers = []() {
    HandlerMap handler_map;

    handler_map[EEventType::kMESSAGE_SEND] = [](const Event& e) {
      const auto& p = std::get<MessageSendPayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] "
                << static_cast<int>(p.from_id.GetIndex()) << " ---> "
                << static_cast<int>(p.to_id.GetIndex()) << " type=\""
                << p.msg.GetType() << "\"" << std::endl;
    };

    handler_map[EEventType::kMESSAGE_RECEIVE] = [](const Event& e) {
      const auto& p = std::get<MessageReceivePayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] "
                << static_cast<int>(p.to_id.GetIndex()) << " <--- "
                << static_cast<int>(p.from_id.GetIndex()) << " type=\""
                << p.msg.GetType() << "\"" << std::endl;
    };

    handler_map[EEventType::kMESSAGE_DROPPED] = [](const Event& e) {
      const auto& p = std::get<MessageDroppedPayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] "
                << static_cast<int>(p.to_id.GetIndex()) << " X--- "
                << static_cast<int>(p.from_id.GetIndex()) << " type=\""
                << p.msg.GetType() << "\"" << std::endl;
    };

    handler_map[EEventType::kLOCAL_MESSAGE] = [](const Event& e) {
      const auto& p = std::get<LocalMessagePayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] "
                << static_cast<int>(p.node_id.GetIndex()) << " <--- local "
                << "type=" << p.msg.GetType() << "\"" << std::endl;
    };

    handler_map[EEventType::kTIMER] = [](const Event& e) {
      const auto& p = std::get<TimerPayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] Timer fired on id"
                << static_cast<int>(p.node_id.GetIndex()) << " name=\""
                << p.timer_name << "\"" << std::endl;
    };

    handler_map[EEventType::kNODE_FAIL] = [](const Event& e) {
      const auto& p = std::get<NodeFailPayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] NODE "
                << static_cast<int>(p.node_id.GetIndex()) << " FAILED"
                << std::endl;
    };

    handler_map[EEventType::kNODE_RECOVER] = [](const Event& e) {
      const auto& p = std::get<NodeRecoverPayload>(e.GetPayload());

      std::cout << "[" << e.GetTimestamp() << "] NODE "
                << static_cast<int>(p.node_id.GetIndex()) << " RECOVERED"
                << std::endl;
    };

    return handler_map;
  }();

  return kDefaultHandlers;
}

Logger Logger::Empty() {
  return Logger();
}

Logger Logger::WithDefaultHandlers() {
  Logger l;

  for (const auto& [type, handler] : DefaultHandlers()) {
    l.RegisterHandler(type, EventHandler(handler));
  }

  return l;
}

void Logger::RegisterHandler(EEventType event_type, EventHandler&& handler) {
  event_handlers_[event_type] = std::move(handler);
}

void Logger::operator()(const Event& event) {
  EEventType curr_event_type = event.GetType();

  auto it = event_handlers_.find(curr_event_type);

  if (it == event_handlers_.end()) {
    return;
  }

  it->second(event);
}

}  // namespace distsysenv
