#pragma once

#include "core/actor/actor_commands.hpp"
#include "core/actor/reflected_actor.hpp"

namespace obcx::actors {

namespace events {

struct ExampleRequested {
  std::string text;
};

struct ExampleBlockingRequested {
  std::string text;
};

struct ExampleHandled {
  std::string text;
};

inline void from_json(const common::json &document, ExampleRequested &message) {
  document.at("text").get_to(message.text);
}

inline void to_json(common::json &document, const ExampleRequested &message) {
  document = {{"text", message.text}};
}

inline void from_json(const common::json &document,
                      ExampleBlockingRequested &message) {
  document.at("text").get_to(message.text);
}

inline void to_json(common::json &document,
                    const ExampleBlockingRequested &message) {
  document = {{"text", message.text}};
}

inline void to_json(common::json &document, const ExampleHandled &message) {
  document = {{"text", message.text}};
}

} // namespace events

namespace commands {
struct ExampleCommand final : command::RequestMessage<ExampleCommand> {};
} // namespace commands

class ExampleActor final : public core::ReflectedActor<ExampleActor> {
public:
  static constexpr std::string_view actor_name = "example";
  static constexpr std::string_view actor_version = "0.1.0";

  static constexpr auto command_contract() {
    return command::catalog(command::observe<commands::ExampleCommand>(
        "example", "Run the example actor command",
        command::re2(R"(^(?:example|example_alias)$)")));
  }

  auto handle(const events::ExampleRequested &request,
              const core::MessageEnvelope &message, core::ActorContext &context)
      -> core::ActorResult;
  auto handle(const events::ExampleBlockingRequested &request,
              const core::MessageEnvelope &message, core::ActorContext &context)
      -> core::ActorTask<core::ActorResult>;
  auto handle(const commands::ExampleCommand &request,
              const core::MessageEnvelope &message, core::ActorContext &context)
      -> core::ActorResult;
};

} // namespace obcx::actors
