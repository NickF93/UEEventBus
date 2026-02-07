# EventBus v2 API Quick Reference

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

### Runtime validation rules

- Channel must be registered before Add/Remove publisher/listener.
- Channel signature is inferred from first publisher delegate bound on the channel.
- New listeners/publishers must be signature-compatible with channel signature.
- Listener identity key is `FObjectKey + FName` (stable across object rename).

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

Typed listener helper contract:

- `NFL_EVENTBUS_METHOD(ClassType, FunctionName)` uses:
- `&ClassType::FunctionName` and `GET_FUNCTION_NAME_CHECKED(ClassType, FunctionName)`.
- No pointer-string parsing workaround is used.

## Blueprint API

- `RegisterChannel`
- `UnregisterChannel`
- `AddPublisherValidated`
- `RemovePublisher`
- `AddListenerValidated`
- `RemoveListener`
- `GetAllowedListenerFunctions`

All validated BP APIs require `UEventBusRegistryAsset` allowlist rules.

Registry model:

- Publisher rules use `TSubclassOf<UObject>` + delegate property name.
- Listener rules use `TSubclassOf<UObject>` + allowed function names.

## C++20 Attribute Aliases

EventBus v2 centralizes attributes in:

- `EventBus/Core/EventBusAttributes.h`

Aliases:

- `NFL_EVENTBUS_MAYBE_UNUSED`
- `NFL_EVENTBUS_NODISCARD`
- `NFL_EVENTBUS_NODISCARD_MSG("reason")`
- `NFL_EVENTBUS_UNUSED(Value)`
