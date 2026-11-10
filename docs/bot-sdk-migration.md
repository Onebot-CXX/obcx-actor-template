# Bot SDK migration

This actor requires OBCX SDK/core `>=2.7.0,<3.0.0` and input contract schema 3. Its scheduler
ABI and `OBCX_ACTOR_EXPORT_V2` remain V2; `package.toml` itself remains manifest
schema 2. A schema-2 actor or old core must be rejected before factory execution,
not accepted because its numeric ABI is still 2.

## Actor-owned scope

- [x] Update package 0.2.0, SDK/core `>=2.7.0,<3.0.0`, CMake minimum SDK 2.7.0 and input contract schema 3.
- [x] Preserve existing synchronous/blocking handlers, event emission, correlated command completion and `Propagation::Consume`; do not add artificial bot/network calls.
- [ ] Verify the new README typed-reply helper against the final frozen SDK headers. The candidate example uses only original typed references and explicit caller-supplied limits; it is documentation, not a new handler.
- [ ] Build with `scripts/actor_build.py` against the matching frozen SDK; verify generated contract/schema gate and record evidence.

The template was built with all six maintained actors against the frozen SDK
2.7.0 under core's external isolated build. Loader/runtime gate acceptance and
the README helper remain separate from that compilation evidence. See core's
`packaging/implementation-handoff.md` for exact product paths and boundaries. No new unit tests, configuration defaults, deployment,
process restart or live database work accompanies this migration.

## Build boundary

Use the core packaging guide to create a new immutable SDK snapshot, then select
`onebot-cxx.example` in the isolated actor build helper. All required workspace,
cache, mode, configuration and logging choices remain explicit. Use at least six
parallel workers, or all available cores when load is low. Do not compile actor
targets in the live core workspace or overwrite a previously frozen SDK.

A compatible rollback restores the matching core/SDK/actor/configuration set;
loading this schema-3 package into an old process is not a rollback path.

## Publication metadata

- [x] Synchronize this actor's entry in the independent registry source
  (`local_actor/actor-registry` in the development workspace), regenerate its
  deterministic metadata-only index with the authoritative package tool, and
  update core's conformance mirror. Do not advertise unbuilt downloads, commit
  publication pins or publish without separate authorization.

The entry is byte-identical to the maintained manifest; both generated indices
passed the authoritative validator and deterministic check. This closes metadata
synchronization only, not the frozen-build/loader/runtime gates above. See core's
`packaging/actors/bot-sdk-cutover.md` for the matching-set rollback inventory.

The registry is publishing metadata, not another actor. Core's `actor-registry/`
is only its conformance snapshot, not a competing metadata authority.
