#include <iostream>
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/event/event.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

namespace distsysenv {

namespace {

struct EchoNode {
  int msg_count = 0;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedMessageEvent dec{
        MessageDeliveryStatus::Sended,
        NodeID(Index{0}, Generation{0}),
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeMessageEvent(e, &dec) != Status::OK ||
        dec.status != MessageDeliveryStatus::Received) {
      return;
    }

    if (dec.msg.GetType() != "hello") {
      return;
    }

    ++msg_count;

    ctx.SendLocal(Message::FromDescription("update", {}));
    ctx.SendMessage(dec.from, Message::FromDescription("echo_reply", {}));
  }
};

struct InvariantChecker {
  int state_updates = 0;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedLocalMessageEvent dec{
        NodeID(Index{0}, Generation{0}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.msg.GetType() != "update") {
      return;
    }

    ++state_updates;

    std::cout << "[CHECKER] Upd" << state_updates << " " << " on time "
              << ctx.Now() << std::endl;
  }
};

}  // namespace

void RunExample() {
  NetworkSettings net_settings{
      .drop_prob = 0.0f,
      .min_delay = 1.0f,
      .max_delay = 5.0f,
  };

  SimulationScenario sim(Network::Config{.behavior = net_settings,
                                         .random_seed = MakeRandomSeed(42)});

  sim.AddNode(InvariantChecker{}, NodeTag::kCHECKER);

  const NodeID a = sim.AddNode(EchoNode{});
  const NodeID b = sim.AddNode(EchoNode{});

  sim.ScheduleMessage(0.0f, a, b, Message::FromDescription("hello", {}));
  sim.ScheduleLocalMessage(0.001f, b, a,
                           Message::FromDescription("local_message", {}));
  sim.ScheduleNodeFail(10.0f, b, a);
  sim.ScheduleNodeRecover(30.0f, b, a);

  sim.RunUntil(50.0f);

  std::cout << "Done" << std::endl;
}

}  // namespace distsysenv

int main() {
  distsysenv::RunExample();

  return 0;
}
