#include <catch2/catch_test_macros.hpp>

#include "src/core/context/context.h"
#include "src/core/event/event.h"
#include "src/core/event/event_manager.h"
#include "src/core/message/message.h"
#include "src/core/node_id/node_id.h"

namespace distsysenv {

namespace {

struct EchoNode {
  int message_count = 0;

  void OnEvent(const Event& event, CoreContext& ctx) {
    message_count++;

    if (message_count < 3) {
      Message reply = Message::FromDescription("echo_reply", {});
      Bytes reply_data = reply.Serialize();

      Event reply_event{ctx.GetOwnID(), event.from, ctx.Now(),
                        std::move(reply_data)};

      ctx.PushEvent(reply_event);
    }
  }
};

struct CounterNode {
  int received = 0;
  std::string last_type;

  void OnEvent(const Event& event, CoreContext& ctx) {
    received++;
    Message msg = Message::Deserialize(event.data);
    last_type = msg.GetType();
  }
};

}  // anonymous namespace

}  // namespace distsysenv

using namespace distsysenv;

TEST_CASE("NodeID generation and management", "[node_id]") {
  NodeIDManager id_manager;

  SECTION("Generate unique IDs") {
    NodeID id1 = id_manager.Generate();
    NodeID id2 = id_manager.Generate();
    NodeID id3 = id_manager.Generate();

    REQUIRE(id1 != id2);
    REQUIRE(id2 != id3);
    REQUIRE(id1 != id3);

  }

  SECTION("Reuse released ID with incremented generation") {
    NodeID id1 = id_manager.Generate();
    NodeID id2 = id_manager.Generate();

    auto old_index = id2.index();
    auto old_generation = id2.generation();

    id_manager.Release(id2);
    NodeID id3 = id_manager.Generate();

    REQUIRE(id3.index() == old_index);
    REQUIRE(id3.generation() != old_generation);
  }

  SECTION("Validate IDs correctly") {
    NodeID id = id_manager.Generate();

    REQUIRE(id_manager.IsValid(id));

    id_manager.Release(id);
    REQUIRE_FALSE(id_manager.IsValid(id));
  }
}

TEST_CASE("NodeID serialization", "[node_id][serialization]") {
  NodeID original(Index{123}, Generation{456});

  Bytes buffer;
  original.Serialize(buffer, 0);

  REQUIRE(buffer.size() == 8);

  NodeID restored = NodeID::Deserialize(buffer, 0);

  REQUIRE(restored == original);
  REQUIRE(restored.index() == original.index());
  REQUIRE(restored.generation() == original.generation());
}

TEST_CASE("Message serialization and deserialization",
          "[message][serialization]") {
  SECTION("Simple message with empty payload") {
    Message msg = Message::FromDescription("GOOOOOOOOL", {});
    Bytes serialized = msg.Serialize();

    REQUIRE_FALSE(serialized.empty());

    Message restored = Message::Deserialize(serialized);

    REQUIRE(restored.GetType() == msg.GetType());
    REQUIRE(restored.GetPayload().size() == msg.GetPayload().size());
  }

  SECTION("Message with complex payload") {
    Bytes payload = BuildPayload(42, 3.14f, std::string("GOOOOOOOOOOOOOOOOL"));
    Message msg =
        Message::FromDescription("GOOOL", std::move(payload));

    Bytes serialized = msg.Serialize();

    REQUIRE(serialized.size() > 0);

    Message restored = Message::Deserialize(serialized);

    REQUIRE(restored.GetType() == msg.GetType());
    REQUIRE(restored.GetPayload().size() == msg.GetPayload().size());

    const auto& orig_payload = msg.GetPayload();
    const auto& rest_payload = restored.GetPayload();
    REQUIRE(orig_payload.size() == rest_payload.size());

    for (size_t i = 0; i < orig_payload.size(); ++i) {
      REQUIRE(orig_payload[i] == rest_payload[i]);
    }
  }
}

TEST_CASE("Event Manager basic functionality", "[event_manager]") {
  EventManager manager;

  SECTION("Register and communicate between nodes") {
    CounterNode node_a;
    CounterNode node_b;

    NodeID id_a = manager.RegisterNode(MakeNodeHandler(std::move(node_a)));
    NodeID id_b = manager.RegisterNode(MakeNodeHandler(std::move(node_b)));

    REQUIRE(id_a != id_b);

    Message msg = Message::FromDescription("hello", {});
    Bytes data = msg.Serialize();

    Event e{id_a, id_b, 0.0f, std::move(data)};
    manager.PushEvent(e);

    manager.Process();
  }
}

TEST_CASE("Event ordering by timestamp", "[event_manager][ordering]") {
  EventManager manager;
  CounterNode node;
  NodeID id = manager.RegisterNode(MakeNodeHandler(std::move(node)));

  Message msg1 = Message::FromDescription("msg_at_10", {});
  manager.PushEvent({id, id, 10.0f, msg1.Serialize()});

  Message msg2 = Message::FromDescription("msg_at_5", {});
  manager.PushEvent({id, id, 5.0f, msg2.Serialize()});

  Message msg3 = Message::FromDescription("msg_at_15", {});
  manager.PushEvent({id, id, 15.0f, msg3.Serialize()});

  manager.ProcessUntil(12.0f);

  REQUIRE(manager.Now() >= 10.0f);
  REQUIRE(manager.Now() <= 12.0f);
}

TEST_CASE("Echo pattern - ping-pong communication",
          "[event_manager][integration]") {
  EventManager manager;

  EchoNode node_1;
  EchoNode node_2;

  NodeID id_1 = manager.RegisterNode(MakeNodeHandler(std::move(node_1)));
  NodeID id_2 = manager.RegisterNode(MakeNodeHandler(std::move(node_2)));

  Message initial = Message::FromDescription("ping", {});
  Bytes data = initial.Serialize();

  Event e{id_1, id_2, 0.0f, std::move(data)};
  manager.PushEvent(e);

  manager.Process();
}

TEST_CASE("NodeID comparison operators", "[node_id]") {
  NodeID id1(Index{1}, Generation{1});
  NodeID id2(Index{2}, Generation{1});
  NodeID id3(Index{1}, Generation{2});
  NodeID id4(Index{1}, Generation{1});

  SECTION("Equality operator") {
    REQUIRE(id1 == id4);
    REQUIRE_FALSE(id1 == id2);
    REQUIRE_FALSE(id1 == id3);
  }

  SECTION("Inequality operator") {
    REQUIRE(id1 != id2);
    REQUIRE(id1 != id3);
    REQUIRE_FALSE(id1 != id4);
  }

  SECTION("Less-than operator for ordering") {
    REQUIRE(id1 < id2);
    REQUIRE(id1 < id3);
  }
}

TEST_CASE("CoreContext basic operations", "[context]") {
  EventManager manager;
  CounterNode node;
  NodeID id = manager.RegisterNode(MakeNodeHandler(std::move(node)));

  Message msg = Message::FromDescription("test", {});
  Event e{id, id, 5.0f, msg.Serialize()};
  manager.PushEvent(e);

  auto status = manager.Step();

  REQUIRE(status == Status::OK);
  REQUIRE(manager.Now() == 5.0f);
}

TEST_CASE("Bytes class wrapper", "[utils][bytes]") {
  SECTION("Basic operations") {
    Bytes buffer;
    REQUIRE(buffer.empty());
    REQUIRE(buffer.size() == 0);

    buffer.resize(10);
    REQUIRE(buffer.size() == 10);
    REQUIRE_FALSE(buffer.empty());
  }

  SECTION("Data access") {
    Bytes buffer(5);
    buffer[0] = std::byte{42};
    buffer[4] = std::byte{99};

    REQUIRE(buffer[0] == std::byte{42});
    REQUIRE(buffer[4] == std::byte{99});
    REQUIRE(buffer.data() != nullptr);
  }

  SECTION("Iterator support") {
    Bytes buffer(3);
    buffer[0] = std::byte{1};
    buffer[1] = std::byte{2};
    buffer[2] = std::byte{3};

    int count = 0;
    for (auto b : buffer) {
      count++;
      REQUIRE(b != std::byte{0});
    }

    REQUIRE(count == 3);
  }
}
