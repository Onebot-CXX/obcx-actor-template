# OBCX Actor Template

This repository is the canonical starting point for a native ABI 2 OBCX actor
package. It uses `ReflectedActor<Derived>`, typed `handle` overloads,
`ActorTask<ActorResult>`, canonical v2 `package.toml` metadata, the installed
OBCX 2.x SDK with actor input contract schema 3, and `OBCXPackages.cmake`.
The package manifest schema and numeric actor ABI stay at 2; neither is the
input-contract schema. It contains no runtime adapter or alternate extension
system. See [bot SDK migration](docs/bot-sdk-migration.md) for the
compatibility boundary and validation status.

## Create An Actor

1. Copy or instantiate this repository as an actor-named project.
2. Update every required field in `package.toml`. Keep `package.kind = "actor"`,
   `actor.abi = 2`, `artifact.kind = "shared-library"`, and
   `actor.entrypoint = "obcx_create_actor_v2"`, and
   `actor.input_contract_schema = 3`. List only built and
   verified release targets in `artifact.platforms`.
3. Rename `ExampleActor` and its source files. Declare `artifact.target`,
   `artifact.export_target` and `artifact.name` in metadata; the constructor
   reads them instead of maintaining another CMake name/output list.
4. Define named message types with nlohmann ADL JSON conversions and implement
   public direct `handle` overloads returning `ActorResult` or
   `ActorTask<ActorResult>`. Keep bounded in-memory work synchronous, use
   `ActorContext::run_blocking` for synchronous database, filesystem, or
   CPU-heavy calls, and use `ActorContext::await_asio` for network, timer, or
   other Asio suspension.
5. For a platform command, derive one request type from
   `command::RequestMessage`, expose it with `command_contract()`, and handle
   that request like any other reflected actor message. Return exactly one
   correlated `command::CommandCompleted`; its `Continue` or `Consume` value
   controls whether the original `RawMessageEvent` enters ordinary pipelines.
   An optional `command::re2(...)` adds normalized full-match aliases while the
   ordinary command name remains the configuration, catalog, header, and
   invocation identity. Do not put a handler name or callable in the command
   declaration.

`OBCX_ACTOR_EXPORT_V2` supplies the numeric ABI generation, factory,
destructor, actor name, actor version, and generated schema-3 input contract.
The runtime validates this contract before actor construction. Name and version
are inherited automatically from `package.toml` (`actor.name`, `package.version`);
do not declare `actor_name` or `actor_version` in C++. The existing one-parameter
`ReflectedActor<Derived>` base and export macro remain sufficient.

Build helpers also attribute `OBCX_INFO/WARN/ERROR` and other logging macros to
`[actor.name]`, including registered internal libraries and background work.
Core-owned code keeps `[core]`. No extra runtime logging configuration or manual
logger injection is required. Do not include another actor package's authoring
facade in the same translation unit; communicate through public message types.

`ExampleRequested` demonstrates the synchronous handler shape.
`ExampleBlockingRequested` demonstrates the blocking boundary:

```cpp
auto value = co_await context.run_blocking([input] {
  return synchronous_library_call(input);
});
```

The callable runs on the process blocking pool and the continuation returns
through the actor scheduler. It must not capture stack references that may
expire, and it must return an owned value rather than a reference or an Asio
awaitable. Do not move timers, sockets, bot sends, or an entire coroutine graph
onto this pool.

## Typed Reply In The Original Conversation

This optional helper is documentation, not an extra handler in the template.
The caller supplies the generation-owned `BotClient`, the admitted typed event
and explicit send limits from its own policy. Passing the original references
preserves the complete conversation scope without reconstructing native routes.

```cpp
#include <core/bot/bot_client.hpp>
#include <core/bot/bot_events.hpp>

namespace example {
auto reply(obcx::bot::BotClient &bot, obcx::bot::MessageReceived received,
           obcx::bot::SendLimits limits)
    -> boost::asio::awaitable<obcx::bot::SendOutcome> {
  co_return co_await bot.send(obcx::bot::SendMessageRequest{
      .target = received.message.conversation,
      .body = {obcx::bot::RichText{obcx::bot::Text{"Received.", {}}}},
      .reply_to = received.message,
      .delivery = obcx::bot::DeliveryPolicy::SingleMessageOnly,
      .limits = limits,
  });
}
} // namespace example
```

An actor awaits this through `context.await_asio(...)`, not `run_blocking`.
Keep the generation-owned client alive throughout the await. Handle both
`SendReceipt` and `SendFailure`; a failure can still contain confirmed messages
and `output_details`, or indicate possible unconfirmed effects. Never treat such
an outcome as a reason to replay the whole request. This helper returns that
complete outcome rather than hiding it behind a success boolean.

## Build And Install

The supported baseline is Linux x86_64/arm64, CMake 3.30+, GCC 16.1+, C++26,
`-freflection`, and `__cpp_impl_reflection >= 202506L`.

Use core's `scripts/actor_build.py` with a frozen, matching installed OBCX 2.x
SDK and an explicit production workspace selecting `onebot-cxx.example`.
CMake resolves the graph offline; see core's `packaging/actors/README.md` and
`docs/architecture/package-cmake.md`. Do not build an actor target in the live
core workspace: transitive rebuilding can replace the running core library.
Compilation uses at least six workers, or all available cores when load is low.
A new input-contract generation needs a matching core rollout; actor-only reload
cannot activate the incompatible SDK. Builds do not deploy or restart anything.

The installed-SDK bootstrap requires all four `OBCX_PACKAGES_WORKSPACE`,
`OBCX_PACKAGES_CACHE`,
`OBCX_PACKAGES_MODE`, `OBCX_PACKAGES_STATE_DIR` variables, an explicit build
configuration and SDK prefix. Keep generated state outside package sources;
expanded standalone distribution acceptance is deferred, not implicitly enabled
by a short `cmake -S .` command.

The shared actor is installed under `lib/obcx/actors/` and its canonical
metadata under `share/obcx/packages/<package-id>/package.toml`.

## Dependencies

`package.toml` is the canonical identity, dependency, compatibility and publication
document. Declare `libraries`, logical `actors`, and `system` dependencies,
including the SDK, with explicit constraints and targets. `test_dependencies`
is separate. `packages.toml` supplies the source/provider bindings.

Use `obcx_add_actor(SOURCES ...)`; external links come from metadata. Internal
implementation and test targets must register with `obcx_package_target` and an
explicit role. Do not manually find/link undeclared external targets.

## Runtime Configuration

The actor name in runtime TOML must match the exported name. Message types are
fully qualified and must match the generated input contract exactly:

```toml
[actors.example]
library = "example"
enabled = true
partition = "conversation_id"

[actors.example.config]
id_prefix = "example"

[command_runtime]
timeout_ms = 5000

[command_runtime.help]
page_bytes = 3500
maximum_pages = 10

[command_runtime.access.groups]
mode = "unrestricted"
entries = []

[command_runtime.access.users]
mode = "unrestricted"
entries = []

[[command_runtime.routes]]
actor = "example"
commands = ["example"]
platforms = ["telegram"]
bots = ["telegram_bot"]
fallback = "continue"

[pipelines.example]
source = "obcx::actors::events::ExampleRequested"

[[pipelines.example.stages]]
name = "handle_example"
actor = "example"
input = "obcx::actors::events::ExampleRequested"
output = "obcx::actors::events::ExampleHandled"
mode = "await"
```

The actor declaration is only a capability description. A route activates it
for explicit platform/bot scopes; the platform adapter detects syntax and the
runtime delivers `commands::ExampleCommand`. Catalog publication, when the
platform supports it, is aggregated by the runtime rather than performed by
the actor. RE2 patterns are matched only after platform syntax and bot-target
validation, are excluded from platform catalogs, and never change which typed
`handle` overload receives the request. Help bounds and both access policies
are explicit whenever routes exist. Core reserves `help`; actors cannot declare
or own it. Use exact platform, bot-installation, and native IDs for scoped
allowlists or denylists; see
[`docs/architecture/actor-command-routing.md`](../../docs/architecture/actor-command-routing.md).

Actor-owned configuration must be read from the immutable generation view:

```cpp
auto id_prefix = context.config()
                     .get_value<std::string>("id_prefix")
                     .value_or("example");
```

Keep any derived settings on the actor instance. Do not call
`ConfigLoader::instance()`, use mutable namespace globals/function statics for
configuration, or read configuration in the factory constructor. Bot
installations, their component registries and transports are process-owned and
are not actor services. Ordinary bot operations use the injected `BotClient`
and typed conversation/message references; only explicit platform SDK bindings
use the controlled gateway internally. Never construct CQ segments, native file
IDs or topic routes in an ordinary reply. This minimal actor has no bot calls and
does not acquire a client or query self merely to demonstrate SDK migration.
Changing bot installation or database-instance definitions requires a separately
authorized process restart.

## Layout

```text
.
├── package.toml
├── CMakeLists.txt
└── src/
    ├── example_actor.cpp
    └── example_actor.hpp
```
