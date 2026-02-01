#include "../src/core/event.h"
#include "../src/core/event_manager.h"
#include "../src/logger/logger.h"
#include "../src/network/network.h"
#include "../src/network/network_settings.h"
#include "../src/node/node_pool.h"
#include "../src/utils/message.h"
#include "echo_node.h"

#include <iostream>
#include <memory>

namespace distsysenv {

static NodeFactory MakeEchoNodeFactory() {
  return [](Mailbox mailbox, TimerManager timer_manager) -> NodeFactoryResult {
    auto node = std::make_shared<EchoNode>(std::move(mailbox),
                                           std::move(timer_manager));
    
    DeliveryFunction deliver = [node](NodeID from_id, const Message& msg) {
      node->OnMessage(from_id, msg);
    };

    TimerDeliveryFunction timer_deliver = [node](const std::string& name) {
      node->OnTimer(name);
    };

    return {std::move(deliver), std::move(timer_deliver)};
  };
}

void RunExample() {
  EventManager events;
  NodePool pool(events);
  Network network(
      events, pool,
      NetworkSettings{.drop_chance = 0.0f, .min_delay = 1, .max_delay = 5},
      RandomSeed{42});

  Logger logger;

  logger.RegisterHandler(EEventType::kMESSAGE_SEND, [](const Event& e) {
    const auto& p = std::get<MessageSendPayload>(e.GetPayload());
    std::cout << "[" << e.GetTimestamp() << "] "
              << static_cast<int>(p.from_id.GetIndex()) << " ---> "
              << static_cast<int>(p.to_id.GetIndex()) << " type=\""
              << p.msg.GetType() << "\"\n";
  });

  logger.RegisterHandler(EEventType::kMESSAGE_RECEIVE, [](const Event& e) {
    const auto& p = std::get<MessageReceivePayload>(e.GetPayload());
    std::cout << "[" << e.GetTimestamp() << "] "
              << static_cast<int>(p.to_id.GetIndex()) << " <--- "
              << static_cast<int>(p.from_id.GetIndex()) << " type=\""
              << p.msg.GetType() << "\"\n";
  });

  logger.RegisterHandler(EEventType::kMESSAGE_DROPPED, [](const Event& e) {
    const auto& p = std::get<MessageDroppedPayload>(e.GetPayload());
    std::cout << "[" << e.GetTimestamp() << "] DROPPED "
              << static_cast<int>(p.from_id.GetIndex()) << " -x-> "
              << static_cast<int>(p.to_id.GetIndex()) << " type=\""
              << p.msg.GetType() << "\"\n";
  });

  logger.RegisterHandler(EEventType::kTIMER, [](const Event& e) {
    const auto& p = std::get<TimerPayload>(e.GetPayload());
    std::cout << "[" << e.GetTimestamp() << "] Timer fired on id"
              << static_cast<int>(p.node_id.GetIndex()) << " name=\""
              << p.timer_name << "\"\n";
  });

  NodeID a = pool.CreateNode(MakeEchoNodeFactory());
  NodeID b = pool.CreateNode(MakeEchoNodeFactory());

  Message echo_msg = Message::FromDescription("echo", {});

  events.Schedule(Event::MessageReceive(1, a, b, echo_msg));

  events.ProcessUntil(100, [&logger, &network](const Event& e) {
    logger(e);
    network.OnEvent(e);
  });

  std::cout << "Example run finished.\n";
}

}  // namespace distsysenv

int main() {
  distsysenv::RunExample();
  
  return 0;
}
