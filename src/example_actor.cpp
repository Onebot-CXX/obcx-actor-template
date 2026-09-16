#include "example_actor.hpp"

#include <algorithm>

namespace obcx::actors {

auto ExampleActor::handle(const events::ExampleRequested &request,
                          const core::MessageEnvelope &message,
                          core::ActorContext &context) -> core::ActorResult {
  context.throw_if_cancelled();
  const auto id_prefix =
      context.config().get_value<std::string>("id_prefix").value_or("example");
  auto result = core::ActorResult::success();
  result.emit(events::ExampleHandled{.text = request.text}, message,
              core::ActorEmitOptions{.id = id_prefix + ":" + message.id});
  return result;
}

auto ExampleActor::handle(const events::ExampleBlockingRequested &request,
                          const core::MessageEnvelope &message,
                          core::ActorContext &context)
    -> core::ActorTask<core::ActorResult> {
  // Sorting stands in for a synchronous CPU/library call. Keep ordinary
  // bounded transforms in a synchronous handler like ExampleRequested.
  auto text = co_await context.run_blocking([text = request.text]() mutable {
    std::ranges::sort(text);
    return text;
  });
  auto result = core::ActorResult::success();
  result.emit(events::ExampleHandled{.text = std::move(text)}, message);
  co_return result;
}

auto ExampleActor::handle(const commands::ExampleCommand &request,
                          const core::MessageEnvelope &message,
                          core::ActorContext &context) -> core::ActorResult {
  context.throw_if_cancelled();
  auto result = core::ActorResult::success();
  result.emit(events::ExampleHandled{.text = request.invocation.arguments},
              message);
  result.emit(
      command::CommandCompleted{
          .transaction_id = request.invocation.transaction_id,
          .propagation = command::Propagation::Consume,
      },
      message);
  return result;
}

} // namespace obcx::actors

OBCX_ACTOR_EXPORT_V2(obcx::actors::ExampleActor)
