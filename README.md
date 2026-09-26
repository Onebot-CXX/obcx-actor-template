# OBCX Actor Template

This repository is the canonical starting point for a native ABI 2 OBCX actor
package. It uses `ReflectedActor<Derived>`, typed `handle` overloads,
`ActorTask<ActorResult>`, canonical v2 `package.toml` metadata, the installed
`obcx-sdk`, and `OBCXPackages.cmake`. It contains no runtime adapter or alternate
extension system.

## Create An Actor

1. Copy or instantiate this repository as an actor-named project.
2. Update every required field in `package.toml`. Keep `package.kind = "actor"`,
   `actor.abi = 2`, `artifact.kind = "shared-library"`, and
   `actor.entrypoint = "obcx_create_actor_v2"`. List only built and
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
destructor, actor name, actor version, and generated schema-2 input contract.
The runtime validates this contract before actor construction.

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

## Build And Install

The supported baseline is Linux x86_64/arm64, CMake 3.30+, GCC 16.1+, C++26,
`-freflection`, and `__cpp_impl_reflection >= 202506L`.

The current development path is an explicit root OBCX workspace selecting this
package. Prepare its frozen graph before configuring; see core's
`docs/architecture/package-cmake.md`. Tests are selected by the workspace profile.

The installed-SDK bootstrap requires all six `OBCX_PACKAGES_WORKSPACE`,
`OBCX_PACKAGES_LOCK`, `OBCX_PACKAGES_GRAPH`, `OBCX_PACKAGES_CACHE`,
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
are not actor services; actors use only `BotOperationGateway` for supported bot
egress. Changing bot installation or database-instance definitions requires a
process restart.

## Layout

```text
.
├── package.toml
├── CMakeLists.txt
└── src/
    ├── example_actor.cpp
    └── example_actor.hpp
```
