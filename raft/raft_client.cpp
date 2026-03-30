#include "raft.h"

#include "message_specs.h"

namespace distsysenv {

void RaftNode::SubmitCommand(const TCommand& command, SimulationContext& ctx) {
  if (role_ != Role::kLEADER) {
    if (leader_id_.has_value()) {
      ForwardCommandToLeader(command, ctx);

    } else {
      pending_client_commands_.push_back(command);
    }
    return;
  }

  ExecuteClientCommand(command, ctx, std::nullopt);
}

void RaftNode::ForwardCommandToLeader(const TCommand& command,
                                      SimulationContext& ctx) {
  if (!leader_id_.has_value()) {
    pending_client_commands_.push_back(command);
    return;
  }

  client_redirect::RequestPayload payload{.reply_to = ctx.GetOwnID(),
                                          .command = command};
  ctx.SendMessage(*leader_id_,
                  Message::FromDescription("client_command_redirected",
                                           payload.Serialize()));
}

void RaftNode::FlushPendingClientCommands(SimulationContext& ctx) {
  if (role_ == Role::kLEADER) {
    while (!pending_client_commands_.empty()) {
      TCommand cmd = std::move(pending_client_commands_.front());
      pending_client_commands_.pop_front();

      ExecuteClientCommand(cmd, ctx, std::nullopt);
    }

    return;
  }

  if (!leader_id_.has_value()) {
    return;
  }

  while (!pending_client_commands_.empty()) {
    TCommand cmd = std::move(pending_client_commands_.front());
    pending_client_commands_.pop_front();
    ForwardCommandToLeader(cmd, ctx);
  }
}

void RaftNode::ExecuteClientCommand(const TCommand& command,
                                    SimulationContext& ctx,
                                    std::optional<NodeID> redirect_to) {
  auto send_response = [&](const command::ResponsePayload& resp_payload) {
    if (redirect_to.has_value()) {
      ctx.SendMessage(*redirect_to, Message::FromDescription(
                                        "client_command_redirected_response",
                                        resp_payload.Serialize()));
    } else {
      ctx.SendLocal(Message::FromDescription("client_command_response",
                                             resp_payload.Serialize()));
    }
  };

  if (command.type() == TCommand::Type::eGET) {
    send_response(command::ResponsePayload{
        .status = Status::OK,
        .command = command,
        .value = kv_store_.Get(command.key()),
    });

    return;
  }

  log_.push_back(LogEntry{.term = current_term_, .command = command});
  const TIndex new_entry_index = GetLastLogIndex();

  pending_client_responses_.push_back(PendingClientResponse{
      .log_index = new_entry_index,
      .redirect_to = redirect_to,
      .command = command,
  });

  for (const auto& peer : peers_) {
    SendAppendEntries(peer, ctx);
  }

  UpdateCommitIndex(ctx);
}

void RaftNode::HandleClientCommandRedirected(NodeID from, const Message& msg,
                                             SimulationContext& ctx) {
  (void)from;
  if (role_ != Role::kLEADER) {
    return;
  }

  client_redirect::RequestPayload payload =
      client_redirect::RequestPayload::Deserialize(msg.GetPayload());

  ExecuteClientCommand(payload.command, ctx, payload.reply_to);
}

void RaftNode::HandleClientCommandRedirectedResponse(NodeID from,
                                                     const Message& msg,
                                                     SimulationContext& ctx) {
  (void)from;
  command::ResponsePayload resp_payload =
      command::ResponsePayload::Deserialize(msg.GetPayload());
  ctx.SendLocal(Message::FromDescription("client_command_response",
                                         resp_payload.Serialize()));
}

void RaftNode::DrainPendingClientResponses(SimulationContext& ctx,
                                           Status response_status,
                                           DrainMode mode) {
  while (!pending_client_responses_.empty()) {
    if (mode == DrainMode::kUpToCommitIndex &&
        pending_client_responses_.front().log_index > commit_index_) {
      break;
    }

    PendingClientResponse pending = std::move(pending_client_responses_.front());
    pending_client_responses_.pop_front();

    const command::ResponsePayload resp_payload{
        .status = response_status,
        .command = std::move(pending.command),
        .value = std::nullopt,
    };
    
    if (pending.redirect_to.has_value()) {
      ctx.SendMessage(
          *pending.redirect_to,
          Message::FromDescription("client_command_redirected_response",
                                   resp_payload.Serialize()));
    } else {
      ctx.SendLocal(Message::FromDescription("client_command_response",
                                             resp_payload.Serialize()));
    }
  }
}

}  // namespace distsysenv
