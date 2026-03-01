#include "../src/core/event.h"
#include "../src/core/event_manager.h"
#include "../src/network/network.h"
#include "../src/network/network_settings.h"
#include "../src/node/context.h"
#include "../src/utils/message.h"

#include <iostream>

namespace distsysenv {

struct EchoNode {
  void OnMessage(NodeID from, const Message& msg, Context& ctx) {
    std::cout << "[" << static_cast<int32_t>(ctx.GetOwnID().GetIndex())
              << "] '" << msg.GetType() 
              << "' from " << static_cast<int32_t>(from.GetIndex()) << std::endl;
    
    ctx.Send(from, Message::FromDescription("echo_reply", {}));
  }

  void OnTimer(const std::string& timer_name, Context& ctx) {
    std::cout << "[" << static_cast<int32_t>(ctx.GetOwnID().GetIndex())
              << "] Timer " << timer_name << std::endl;
  }
};

void RunExample() {
  EventManager manager;

  NetworkSettings net_settings{
      .drop_prob = 0.0f,
      .min_delay = 1,
      .max_delay = 5
  };
  Network network(net_settings, 42);
  manager.RegisterNetwork(std::move(network));

  EchoNode node_a;
  NodeID a = manager.RegisterNode(std::move(node_a));

  EchoNode node_b;
  NodeID b = manager.RegisterNode(std::move(node_b));

  manager.Schedule(
      Event::MessageSend(0, a, b, Message::FromDescription("hello", {}))
  );

  manager.ProcessUntil(100);

  std::cout << "Done" << std::endl;
}

}  // namespace distsysenv

int main() {
  distsysenv::RunExample();

  return 0;
}
