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
2. Typed (`EventBus/Typed/*`): compile-time channel utilities for C++.
3. BP (`EventBus/BP/*`): subsystem and blueprint function facade.
4. Registry (`UEventBusRegistryAsset`): allowlist governance for BP operations.

## Dependency DAG

- Core -> none
- Typed -> Core
- BP -> Core + Registry
- Registry -> Core types only

No reverse dependencies are allowed.

## Runtime Flow

1. Register channel with ownership policy.
2. Add publisher (channel + object + delegate property).
3. Add listener (channel + object + function name).
4. Publisher broadcasts delegate.
5. Remove listener/publisher as needed.
6. Reset or unregister channel performs full unbind and cleanup.

## Channel Signature Model

1. First valid publisher delegate bound on a channel establishes the channel signature.
2. Additional publishers on the same channel must be signature-compatible.
3. New listeners must be signature-compatible with established channel signature.
4. Listener-first registration is allowed; compatibility is enforced when first publisher is added.

## Listener Identity Model

1. Listener identity key is `FObjectKey + FName`.
2. Identity is stable across object rename operations.
3. Remove operations use this key in non-owning mode.
4. Owning mode removal still supports object-wide callback cleanup semantics.

## Blueprint Registry Model

1. Publisher and listener class allowlists are defined with `TSubclassOf<UObject>`.
2. Rules are deterministic at runtime (no lazy-load soft class ambiguity).
3. Validation is mandatory for Blueprint add/remove bind entry points.

## Ownership Policy

- `bOwnsPublisherDelegates = true`
  - removal uses object-wide EventBus-managed callback cleanup.
- `bOwnsPublisherDelegates = false`
  - removal uses targeted callback cleanup.

## Threading

All API calls are game-thread only.
