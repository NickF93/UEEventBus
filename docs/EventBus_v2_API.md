# EventBus v2 API Reference

## Engineering Constraints

Use the project-level constraints defined in `README.md`.

## Core Runtime API

```cpp
Nfrrlib::EventBus::FEventBus Bus;

Bus.RegisterChannel({ChannelTag, /*bOwnsPublisherDelegates=*/false});
Bus.AddPublisher(ChannelTag, PublisherObj, {DelegatePropertyName});
Bus.AddListener(ChannelTag, ListenerObj, {FunctionName});
Bus.RemoveListener(ChannelTag, ListenerObj, {FunctionName});
Bus.RemovePublisher(ChannelTag, PublisherObj);
Bus.UnregisterChannel(ChannelTag);
Bus.Reset();
```

### Runtime Rules

- Channel must be registered before add/remove publisher/listener calls.
- For game-wide channels, prefer registration during game-instance startup (for example `UGameInstanceSubsystem::Initialize`) to avoid actor `BeginPlay` ordering races.
- Channel signature is inferred from first valid publisher delegate bound on that channel.
- Listener/publisher bindings must remain signature-compatible.
- Listener identity key is `FObjectKey + FName`.

## Typed C++ API

```cpp
NFL_DECLARE_EVENTBUS_CHANNEL(FMyChannel, FMyDelegate, UMyPublisher, TAG_MyChannel, OnMyEvent);

using namespace Nfrrlib::EventBus;

FEventBus Bus;
TEventChannelApi<FMyChannel>::Register(Bus, false);
TEventChannelApi<FMyChannel>::AddPublisher(Bus, Publisher);
NFL_EVENTBUS_ADD_LISTENER(Bus, FMyChannel, Listener, UMyListenerClass, OnMyEvent);
NFL_EVENTBUS_REMOVE_LISTENER(Bus, FMyChannel, Listener, UMyListenerClass, OnMyEvent);
```

Typed helper contract:

- `NFL_EVENTBUS_METHOD(ClassType, FunctionName)` resolves pointer + checked name.
- No pointer-string parsing is used.

## Blueprint API

`UEventBusBlueprintLibrary`:

- `RegisterChannel`
- `UnregisterChannel`
- `AddPublisherValidated`
- `AddPublisher`
- `RemovePublisher`
- `AddListenerValidated`
- `AddListener`
- `RemoveListener`
- `GetKnownListenerFunctions`

All binding add methods (`AddPublisherValidated`, `AddPublisher`, `AddListenerValidated`, `AddListener`) do runtime checks and record successful binds into runtime history.
Runtime history auto-prunes invalid class entries and uses a bounded in-memory size.
`UEventBusRegistryAsset::ResetHistory()` clears all runtime history explicitly.
No pre-authored rule table setup is required.

## Editor Filtered Nodes

- `Add Publisher Validated (Filtered)`:
  - delegate picker lists delegates declared on selected publisher class only.
- `Add Listener Validated (Filtered)`:
  - function picker lists functions declared on selected listener class only.

## C++20 Attribute Aliases

`EventBus/Core/EventBusAttributes.h`:

- `NFL_EVENTBUS_MAYBE_UNUSED`
- `NFL_EVENTBUS_NODISCARD`
- `NFL_EVENTBUS_NODISCARD_MSG(...)`
- `NFL_EVENTBUS_UNUSED(Value)`
