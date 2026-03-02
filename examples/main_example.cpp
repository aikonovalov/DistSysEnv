#include "../src/core/event.h"
#include "../src/core/event_manager.h"
#include "../src/logger/logger.h"
#include "../src/network/network.h"
#include "../src/network/network_settings.h"
#include "../src/node/context.h"
#include "../src/utils/message.h"

#include <iostream>

namespace distsysenv {

struct EchoNode {
  int msg_count = 0;

  void OnMessage(NodeID from, const Message& msg, Context& ctx) {
    msg_count++;

    ctx.SendLocal(Message::FromDescription("update", {}));

    ctx.Send(from, Message::FromDescription("echo_reply", {}));
  }

  void OnLocalMessage(const Message& msg, Context& ctx) {
    (void)msg;
    (void)ctx;
  }

  void OnTimer(const std::string& timer_name, Context& ctx) {
    (void)timer_name;
    (void)ctx;
  }
};

struct InvariantChecker {
  int state_updates = 0;

  void OnLocalMessage(const Message& msg, Context& ctx) {
    if (msg.GetType() == "update") {
      state_updates++;

      std::cout << "[CHECKER] Upd" << state_updates << " " << ctx.Now()
                << std::endl;
    }
  }
};

void RunExample() {
  EventManager manager;

  Logger logger = Logger::WithDefaultHandlers();
  manager.SetLogger(std::move(logger));

  NetworkSettings net_settings{
      .drop_prob = 0.0f, .min_delay = 1, .max_delay = 5};
  Network network(net_settings, 42);
  manager.RegisterNetwork(std::move(network));

  InvariantChecker checker;
  manager.RegisterChecker(std::move(checker));

  EchoNode node_a;
  NodeID a = manager.RegisterNode(std::move(node_a));

  EchoNode node_b;
  NodeID b = manager.RegisterNode(std::move(node_b));

  manager.Schedule(
      Event::MessageSend(0, a, b, Message::FromDescription("hello", {})));

  manager.SendLocal(b, Message::FromDescription("local_message", {}));

  manager.Schedule(Event::NodeFail(10, b));

  manager.Schedule(Event::NodeRecover(30, b));

  manager.ProcessUntil(50);

  std::cout << "Done" << std::endl;
}

}  // namespace distsysenv

int main() {
  distsysenv::RunExample();

  return 0;
}
