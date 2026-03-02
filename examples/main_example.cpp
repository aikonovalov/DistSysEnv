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
  void OnMessage(NodeID from, const Message& msg, Context& ctx) {
    (void)msg;
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

void RunExample() {
  EventManager manager;

  Logger logger = Logger::WithDefaultHandlers();
  manager.SetLogger(std::move(logger));

  NetworkSettings net_settings{
      .drop_prob = 0.0f, .min_delay = 1, .max_delay = 5};
  Network network(net_settings, 42);
  manager.RegisterNetwork(std::move(network));

  EchoNode node_a;
  NodeID a = manager.RegisterNode(std::move(node_a));

  EchoNode node_b;
  NodeID b = manager.RegisterNode(std::move(node_b));

  manager.Schedule(
      Event::MessageSend(0, a, b, Message::FromDescription("hello", {})));
  
  manager.SendLocal(b, Message::FromDescription("local_message", {}));

  manager.ProcessUntil(20);

  std::cout << "Done" << std::endl;
}

}  // namespace distsysenv

int main() {
  distsysenv::RunExample();

  return 0;
}
