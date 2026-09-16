# OBCX Actor Template

This repository is the canonical starting point for a native ABI 2 OBCX actor
package. It uses `ReflectedActor<Derived>`, typed `handle` overloads,
`ActorTask<ActorResult>`, canonical `actor.toml` metadata, the installed
`obcx-sdk`, and `OBCXActor.cmake`. It contains no runtime adapter or alternate
extension system.

## Create An Actor

1. Copy or instantiate this repository as an actor-named project.
2. Update every required field in `actor.toml`. Keep `actor.abi = 2`,
   `artifact.kind = "shared-library"`, and
   `artifact.entrypoint = "obcx_create_actor_v2"`. List only built and
   verified release targets in `artifact.platforms`.
3. Rename `ExampleActor`, its source files, and the `obcx_add_actor` target.
   `artifact.target` must equal `<name>_actor`; `artifact.name` must equal the
   helper's `OUTPUT_NAME`.
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

Install OBCX or point CMake to an SDK prefix, then run:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/obcx-sdk-prefix
cmake --build build
cmake --install build --prefix /path/to/actor-prefix
```

The shared actor is installed under `lib/obcx/actors/` and its canonical
metadata under `share/obcx/actors/<actor-id>/actor.toml`.

## Dependencies

`actor.toml` is the canonical identity, dependency, compatibility, and
publication document. Declare package-manager dependency names only in
`[dependencies].packages` and actor dependencies only in
`[dependencies].actors`. The consuming OBCX checkout merges package
dependencies from every package selected in its `actors.toml`; this standalone
template repository does not contain or invoke the OBCX manifest generator.

Use ordinary `find_package()` calls and pass linked CMake targets through the
`DEPS` argument of `obcx_add_actor`.

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
├── actor.toml
├── CMakeLists.txt
└── src/
    ├── example_actor.cpp
    └── example_actor.hpp
```
