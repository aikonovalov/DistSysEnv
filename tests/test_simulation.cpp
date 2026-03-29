#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include "src/core/event/event.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"
#include "src/simulation/event/event.h"
#include "src/simulation/context/context.h"
#include "src/simulation/scenario/scenario.h"
#include "src/utils/random.h"

namespace distsysenv {

namespace {

struct ProcessWithTerminalOutput {
  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    if (!e.data.empty()) {
      return;
    }

    ctx.SendLocal(Message::FromDescription("invariant_ping", {}));
  }
};

struct UnifiedTerminalNode {
  std::shared_ptr<int> lines_from_other_processes;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedLocalMessageEvent dec{NodeID(Index{0}, Generation{1}),
                                 Message::FromDescription("_", {})};

    if (TryDecodeLocalMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.from != ctx.GetOwnID()) {
      ++*lines_from_other_processes;
    }
  }
};

struct TerminalInjectingCommand {
  NodeID target_process;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    if (!e.data.empty()) {
      return;
    }

    ctx.SendLocal(target_process, Message::FromDescription("repl_cmd", {}));
  }
};

struct ProcessAcceptingNetworkMessages {
  std::shared_ptr<int> commands_seen;

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    DecodedMessageEvent dec{
        MessageDeliveryStatus::Sended,
        NodeID(Index{0}, Generation{1}),
        NodeID(Index{0}, Generation{1}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.msg.GetType() == "repl_cmd") {
      ++*commands_seen;
    }
  }
};

struct NetworkGatewaySpy {
  std::shared_ptr<std::vector<NodeID>> envelope_to;

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    envelope_to->push_back(e.to);
  }
};

struct PeerSenderProcess {
  NodeID peer = NodeID(Index{9}, Generation{1});
  void OnSimulationEvent(const Event&, SimulationContext& ctx) {
    ctx.SendMessage(peer, Message::FromDescription("hello_peer", {}));
  }
};

}  // namespace

}  // namespace distsysenv

using namespace distsysenv;

TEST_CASE("LocalMessage wire: process writes to terminal, not to self",
          "[simulation][event]") {
  const NodeID process(Index{5}, Generation{2});
  const NodeID terminal(Index{7}, Generation{1});
  Message msg = Message::FromDescription("log_line", {});

  LocalMessageEventPayload payload{process, msg};
  Event ev = SimulationEvent::make<LocalMessageEventPayload>::Of(
      2.0f, terminal, std::move(payload));

  SimulationEventKind kind{};
  REQUIRE(ClassifySimulationEvent(ev, &kind) == Status::OK);
  REQUIRE(kind == SimulationEventKind::LocalMessage);

  DecodedLocalMessageEvent out{NodeID(Index{0}, Generation{1}),
                               Message::FromDescription("_", {})};

  REQUIRE(TryDecodeLocalMessageEvent(ev, &out) == Status::OK);
  REQUIRE(out.from == process);
  REQUIRE(ev.to == terminal);
  REQUIRE(out.msg.GetType() == "log_line");
}

TEST_CASE("Message wire: network envelope between nodes",
          "[simulation][event]") {
  const NodeID from(Index{1}, Generation{1});
  const NodeID to(Index{2}, Generation{1});
  Message msg = Message::FromDescription("kind_a", BuildPayload(int32_t{7}));

  MessageEventPayload payload{MessageDeliveryStatus::Sended, from, to, msg};
  Event ev =
      SimulationEvent::make<MessageEventPayload>::Of(3.5f, std::move(payload));

  SimulationEventKind kind{};
  REQUIRE(ClassifySimulationEvent(ev, &kind) == Status::OK);
  REQUIRE(kind == SimulationEventKind::Message);

  DecodedMessageEvent out{
      MessageDeliveryStatus::Sended,
      NodeID(Index{0}, Generation{1}),
      NodeID(Index{0}, Generation{1}),
      Message::FromDescription("_", {}),
  };

  REQUIRE(TryDecodeMessageEvent(ev, &out) == Status::OK);
  REQUIRE(out.status == MessageDeliveryStatus::Sended);
  REQUIRE(out.from == from);
  REQUIRE(out.to == to);
  REQUIRE(out.msg.GetType() == "kind_a");
}

TEST_CASE("Routed Message: Event.to is gateway, logical peer stays in payload",
          "[simulation][routing]") {
  const NodeID app(Index{3}, Generation{1});
  const NodeID gateway(Index{100}, Generation{1});
  const NodeID logical_peer(Index{4}, Generation{1});

  Message msg = Message::FromDescription("trace", {});
  MessageEventPayload payload{MessageDeliveryStatus::Sended, app, logical_peer,
                              msg};

  Event ev = MakeRoutedApplicationMessage(1.0f, gateway, std::move(payload));

  REQUIRE(ev.from == app);
  REQUIRE(ev.to == gateway);
  REQUIRE(ev.timestamp == 1.0f);

  DecodedMessageEvent dec{
      MessageDeliveryStatus::Sended,
      NodeID(Index{0}, Generation{1}),
      NodeID(Index{0}, Generation{1}),
      Message::FromDescription("_", {}),
  };

  REQUIRE(TryDecodeMessageEvent(ev, &dec) == Status::OK);
  REQUIRE(dec.from == app);
  REQUIRE(dec.to == logical_peer);
  REQUIRE(dec.msg.GetType() == "trace");
}

TEST_CASE("SendLocal(msg) reaches unified terminal only",
          "[simulation][terminal]") {
  SimulationScenario sim;

  auto line_count = std::make_shared<int>(0);
  sim.AddNode(UnifiedTerminalNode{line_count}, NodeTag::kCHECKER);
  const NodeID process_id = sim.AddNode(ProcessWithTerminalOutput{});

  sim.Manager().PushEvent(Event{process_id, process_id, 0.0f, Bytes{}});
  sim.Manager().Process();

  REQUIRE(*line_count == 1);
}

TEST_CASE("SendLocal(to, msg) injects from terminal into process",
          "[simulation][repl]") {
  SimulationScenario sim;

  auto seen = std::make_shared<int>(0);
  const NodeID process_id = sim.AddNode(ProcessAcceptingNetworkMessages{seen});
  const NodeID terminal_id = sim.AddNode(TerminalInjectingCommand{process_id});

  sim.Manager().PushEvent(Event{terminal_id, terminal_id, 0.0f, Bytes{}});
  sim.Manager().Process();

  REQUIRE(*seen == 1);
}

TEST_CASE("SendMessage uses network_gateway as first hop",
          "[simulation][routing]") {
  SimulationScenario sim;

  auto log = std::make_shared<std::vector<NodeID>>();
  const NodeID gw_id =
      sim.AddNode(NetworkGatewaySpy{log}, NodeTag::kNETWORK_GATEWAY);
  const NodeID sender_id = sim.AddNode(PeerSenderProcess{});

  sim.Manager().PushEvent(Event{sender_id, sender_id, 0.0f, Bytes{}});
  sim.Manager().Process();

  REQUIRE_FALSE(log->empty());
  REQUIRE((*log)[0] == gw_id);
}

namespace {

struct CountReceivedOfType {
  std::shared_ptr<int> count;
  std::string want_type;

  void OnSimulationEvent(const Event& e, SimulationContext&) {
    DecodedMessageEvent dec{
        MessageDeliveryStatus::Sended,
        NodeID(Index{0}, Generation{1}),
        NodeID(Index{0}, Generation{1}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeMessageEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.status == MessageDeliveryStatus::Received &&
        dec.msg.GetType() == want_type) {
      ++*count;
    }
  }
};

struct SendToPeerOnWake {
  NodeID peer;

  void OnSimulationEvent(const Event&, SimulationContext& ctx) {
    ctx.SendMessage(peer, Message::FromDescription("hello_peer", {}));
  }
};

}  // namespace

TEST_CASE("Network gateway delivers Message to logical peer",
          "[simulation][network]") {
  NetworkSettings net_settings;
  net_settings.drop_prob = 0.0f;
  net_settings.min_delay = 0.0f;
  net_settings.max_delay = 0.0f;

  SimulationScenario sim(Network::Config{.behavior = net_settings,
                                         .random_seed = MakeRandomSeed(42)});

  auto received = std::make_shared<int>(0);
  const NodeID recv_id =
      sim.AddNode(CountReceivedOfType{received, "hello_peer"});
  const NodeID sender_id = sim.AddNode(SendToPeerOnWake{recv_id});

  sim.Manager().PushEvent(Event{sender_id, sender_id, 0.0f, Bytes{}});
  sim.Manager().Process();

  REQUIRE(*received == 1);
}

namespace {

struct SendToPeerAndCountFailed {
  NodeID peer;
  std::shared_ptr<int> failed_count;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    DecodedMessageEvent dec{
        MessageDeliveryStatus::Sended,
        NodeID(Index{0}, Generation{1}),
        NodeID(Index{0}, Generation{1}),
        Message::FromDescription("_", {}),
    };

    if (TryDecodeMessageEvent(e, &dec) == Status::OK &&
        dec.status == MessageDeliveryStatus::Failed) {
      ++*failed_count;

      return;
    }

    if (!e.data.empty()) {
      return;
    }

    ctx.SendMessage(peer, Message::FromDescription("x", {}));
  }
};

}  // namespace

TEST_CASE("Network drop notifies sender with Failed", "[simulation][network]") {
  NetworkSettings net_settings;
  net_settings.drop_prob = 1.0f;

  SimulationScenario sim(Network::Config{.behavior = net_settings,
                                         .random_seed = MakeRandomSeed(7)});

  struct IgnoreAll {
    void OnSimulationEvent(const Event&, SimulationContext&) {}
  };

  const NodeID recv_id = sim.AddNode(IgnoreAll{});

  auto failed_count = std::make_shared<int>(0);
  const NodeID sender_id =
      sim.AddNode(SendToPeerAndCountFailed{recv_id, failed_count});

  sim.Manager().PushEvent(Event{sender_id, sender_id, 0.0f, Bytes{}});
  sim.Manager().Process();

  REQUIRE(*failed_count == 1);
}

namespace {

struct TimerResetScenarioNode {
  std::shared_ptr<int> valid_fires;
  std::shared_ptr<int> stale_fires_ignored;
  std::shared_ptr<int> wake_step;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    if (e.data.empty()) {
      const int step = *wake_step;
      if (step == 0) {
        ctx.SetTimer("GOOOL", 100.0f);
        *wake_step = 1;

      } else if (step == 1) {
        ctx.SetTimer("GOOOL", 50.0f);
        *wake_step = 2;

      }

      return;
    }

    SimulationEventKind kind{};
    if (ClassifySimulationEvent(e, &kind) != Status::OK) {
      return;
    }

    if (kind != SimulationEventKind::Timer) {
      return;
    }

    if (e.to != ctx.GetOwnID()) {
      return;
    }

    DecodedTimerEvent dec{};
    if (TryDecodeTimerEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.name != "GOOOL") {
      return;
    }

    if (!ctx.IsTimerValid(dec.name, dec.token)) {
      ++*stale_fires_ignored;
      return;
    }

    ++*valid_fires;
  }
};

struct TimerCancelOnlyNode {
  std::shared_ptr<int> valid_fires;
  std::shared_ptr<int> stale_fires;
  std::shared_ptr<int> wake_step;

  void OnSimulationEvent(const Event& e, SimulationContext& ctx) {
    if (e.data.empty()) {
      const int s = *wake_step;
      if (s == 0) {
        ctx.SetTimer("GOOOL", 80.0f);
        *wake_step = 1;

      } else if (s == 1) {
        ctx.CancelTimer("GOOOL");
        *wake_step = 2;

      }
      return;
    }

    SimulationEventKind kind{};
    if (ClassifySimulationEvent(e, &kind) != Status::OK) {
      return;
    }

    if (kind != SimulationEventKind::Timer) {
      return;
    }

    if (e.to != ctx.GetOwnID()) {
      return;
    }

    DecodedTimerEvent dec{};
    if (TryDecodeTimerEvent(e, &dec) != Status::OK) {
      return;
    }

    if (dec.name != "GOOOL") {
      return;
    }

    if (!ctx.IsTimerValid(dec.name, dec.token)) {
      ++*stale_fires;
      return;
    }

    ++*valid_fires;
  }
};

}

TEST_CASE("Timer reschedule: stale fire ignored, latest fire handled",
          "[simulation][timer]") {
  SimulationScenario sim;

  auto valid = std::make_shared<int>(0);
  auto stale = std::make_shared<int>(0);
  auto step = std::make_shared<int>(0);

  const NodeID node_id =
      sim.AddNode(TimerResetScenarioNode{valid, stale, step});

  sim.Manager().PushEvent(Event{node_id, node_id, 0.0f, Bytes{}});
  sim.Manager().PushEvent(Event{node_id, node_id, 10.0f, Bytes{}});

  sim.RunUntil(110.0f);

  REQUIRE(*valid == 1);
  REQUIRE(*stale == 1);
  REQUIRE(*step == 2);
}

TEST_CASE("CancelTimer invalidates pending fire without rescheduling",
          "[simulation][timer]") {
  SimulationScenario sim;

  auto valid = std::make_shared<int>(0);
  auto stale = std::make_shared<int>(0);
  auto step = std::make_shared<int>(0);

  const NodeID id = sim.AddNode(TimerCancelOnlyNode{valid, stale, step});

  sim.Manager().PushEvent(Event{id, id, 0.0f, Bytes{}});
  sim.Manager().PushEvent(Event{id, id, 5.0f, Bytes{}});

  sim.RunUntil(100.0f);

  REQUIRE(*valid == 0);
  REQUIRE(*stale == 1);
  REQUIRE(*step == 2);
}
