# EventBus v2 Architecture

## Mandatory Constraints (1-14)

1. Unreal Engine 5.5+ coding rules and best practices.
2. C++20+ best practices and standards.
3. Prefer Unreal Engine libraries/types over STL where practical.
4. Use well-defined GoF patterns where appropriate.
5. Maintain strong decoupling.
6. Keep naming and formatting coherent.
7. Keep comments, doxygen, and textual docs up to date.
8. Enforce clear responsibility separation between code entities.
9. Follow OOP best practices.
10. Maintain compatibility with C++ `>= 20` and Unreal Engine `>= 5.5` (5.6/5.7+ included).
11. Keep code clean, safe, and well structured.
12. Avoid workaround and non-canonical code paths.
13. Apply DRY best practices and avoid unnecessary repetition.
14. Avoid dead or unreachable code.

## Layers

1. Core (`EventBus/Core/*`): runtime orchestration and channel state.
2. Typed (`EventBus/Typed/*`): compile-time channel wrappers for native C++.
3. Blueprint (`EventBus/BP/*`): subsystem + function library.
4. Editor (`EventBusEditor/*`): filtered K2 nodes and pin factories.

## Dependency DAG

- Core -> none
- Typed -> Core
- Blueprint -> Core
- Editor -> Blueprint (+ Unreal editor modules)

No reverse dependencies are allowed.

## Runtime Flow

1. Register one channel with ownership policy.
2. Add one or more publishers on that channel.
3. Add one or more listeners on that channel.
4. Dispatch via publisher delegate.
5. Remove listener/publisher entries.
6. Unregister channel or reset subsystem.

## Channel Signature Model

1. First successful publisher bind defines channel signature.
2. Additional publishers/listeners on that channel must be compatible.
3. Listener-first registration is allowed; first publisher enforces compatibility.

## Listener Identity Model

1. Listener identity key is `FObjectKey + FName`.
2. Remove operations target this key in non-owning mode.
3. Owning mode can perform object-wide EventBus callback cleanup.

## Runtime History Model

1. Subsystem creates an internal transient runtime history object at initialize.
2. Successful BP binds are recorded as channel/class/member tuples.
3. `GetKnownListenerFunctions` reads this dynamic history.
4. Runtime history auto-prunes invalid class entries and remains bounded in size.
5. `UEventBusRegistryAsset::ResetHistory()` can clear all runtime history on demand.
6. No manual rule asset setup is required.

## Ownership Policy

- `bOwnsPublisherDelegates = true`
  - object-wide EventBus-managed callback cleanup.
- `bOwnsPublisherDelegates = false`
  - exact callback cleanup only.

## Threading

All runtime API calls are game-thread only.
