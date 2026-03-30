#include "raft.h"

#include "../src/simulation/event/event.h"
#include "message_specs.h"

namespace distsysenv {

RaftNode::RaftNode(std::vector<NodeID> peers) : peers_(std::move(peers)) {}

void RaftNode::SetPeers(std::vector<NodeID> new_peers) {
  peers_ = std::move(new_peers);
}

void RaftNode::OnSimulationEvent(const Event& e, SimulationContext& ctx) {
  SimulationEventKind kind{};
  Status op_status = ClassifySimulationEvent(e, &kind);
  if (op_status != Status::OK) {
    return;
  }

  switch (kind) {
    case SimulationEventKind::Message: {
      DecodedMessageEvent dec{
          .from = ctx.GetOwnID(),
          .to = ctx.GetOwnID(),
          .msg = Message::FromDescription("_", {}),
      };

      op_status = TryDecodeMessageEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      if (dec.status != MessageDeliveryStatus::Received) {
        return;
      }
      if (dec.to != ctx.GetOwnID()) {
        return;
      }

      OnMessage(dec.from, dec.msg, ctx);

      return;
    }
    case SimulationEventKind::LocalMessage: {
      DecodedLocalMessageEvent dec{NodeID(Index{0}, Generation{0}),
                                   Message::FromDescription("_", {})};

      op_status = TryDecodeLocalMessageEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      OnLocalMessage(dec.msg, ctx);

      return;
    }
    case SimulationEventKind::Timer: {
      if (e.to != ctx.GetOwnID()) {
        return;
      }

      DecodedTimerEvent dec{};
      op_status = TryDecodeTimerEvent(e, &dec);
      if (op_status != Status::OK) {
        return;
      }

      if (!ctx.IsTimerValid(dec.name, dec.token)) {
        return;
      }

      OnTimer(dec.name, ctx);

      return;
    }
    case SimulationEventKind::NodeStatus: {
      return;
    }
    case SimulationEventKind::PartitionPair: {
      return;
    }
  }
}

void RaftNode::OnMessage(NodeID from, const Message& msg,
                         SimulationContext& ctx) {
  MessageType curr_type = msg.GetType();

  if (curr_type == "RequestVote") {
    HandleRequestVote(from, msg, ctx);

  } else if (curr_type == "RequestVoteResponse") {
    HandleRequestVoteResponse(from, msg, ctx);

  } else if (curr_type == "AppendEntries") {
    HandleAppendEntries(from, msg, ctx);

  } else if (curr_type == "AppendEntriesResponse") {
    HandleAppendEntriesResponse(from, msg, ctx);
  } else if (curr_type == "client_command_redirected") {
    HandleClientCommandRedirected(from, msg, ctx);
  } else if (curr_type == "client_command_redirected_response") {
    HandleClientCommandRedirectedResponse(from, msg, ctx);
  }
}

void RaftNode::OnLocalMessage(const Message& msg, SimulationContext& ctx) {
  MessageType curr_type = msg.GetType();

  if (curr_type == "set_peers") {
    SetPeers(peers_list::RequestPayload::Deserialize(msg.GetPayload()).peers);

  } else if (curr_type == "client_command") {
    TOffset offset = 0;
    TCommand command = TCommand::Deserialize(msg.GetPayload(), offset);
    SubmitCommand(command, ctx);

  } else if (curr_type == "start") {
    ResetElectionTimer(ctx);
  }
}

void RaftNode::OnTimer(const std::string& timer_name, SimulationContext& ctx) {
  if (timer_name == "election") {
    StartElection(ctx);

  } else if (timer_name == "heartbeat") {
    if (role_ == Role::kLEADER) {
      SendHeartbeats(ctx);
      ctx.SetTimer("heartbeat", kHEARTBEAT_INTERVAL);
    }
  }
}

}  // namespace distsysenv
