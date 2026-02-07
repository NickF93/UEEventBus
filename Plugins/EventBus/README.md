# EventBus Plugin

A lightweight, gameplay-tag-driven event bus for Unreal Engine 5.5. The system connects **publishers** (UObjects that expose multicast delegates) to **listeners** (UObjects that bind UFUNCTION callbacks). The API supports both strongly typed compile-time tags and runtime/Blueprint routing through `FGameplayTag` channels.

---

## Highlights

- **Typed event tags** via `NFL_DECLARE_EVENT_TAG` for compile-time safety and clear intent.
- **Runtime channel routing** for dynamic or Blueprint-driven setups.
- **Deterministic listener IDs** (no manual ID storage required).
- **Multiple publishers per channel** with automatic rebind on registration changes.
- **Configurable delegate ownership** (`bOwnsPublisherDelegates`) for safe removal semantics.
- **Game-thread enforcement** and defensive validation using `Nfrrlib::IsValid`.

---

## Requirements

- Unreal Engine **5.5**
- **C++20 is required** for the core EventBus API (concepts are used in public headers). `Util.h` retains a C++17 fallback for validity helpers only.
- Gameplay Tags module enabled

---

## Installation

1. The plugin should be enabled in the project (Plugins panel or `.uproject`).
2. The module dependency should be added:

    ```cpp
    // <Project>.Build.cs
    PublicDependencyModuleNames.AddRange(new string[]
    {
        "Core",
        "CoreUObject",
        "Engine",
        "GameplayTags",
        "EventBus"
    });
    ```

3. Public headers can be included as needed:

    ```cpp
    #include "EventBus/EventBusManager.h"
    #include "EventBus/EventTags.h"
    ```

---

## Concepts

### Publisher

- Any `UObject` that owns a multicast delegate (e.g., `DECLARE_DYNAMIC_MULTICAST_DELEGATE_...`).
- Publishers are registered to a **channel** identified by `FGameplayTag`.

### Listener

- Any `UObject` that exposes a `UFUNCTION` matching the delegate signature.
- Listeners are registered to a channel and automatically bound to all current and future publishers on that channel.

### Channel

- A gameplay tag used to route events.
- One channel maps to one route (typed or runtime). Conflicting routes are rejected with a warning.

---

## Use Cases

- **Gameplay events**: stats changes, ability triggers, quest updates, achievements.
- **UI notifications**: update widgets without hard references to the publisher.
- **Debug and telemetry**: hook debug listeners to channels at runtime.
- **Modular systems**: decouple publishers and listeners across modules or plugins.

---

## Quick Start (C++)

### 1) Define gameplay tags

```cpp
// MyNativeTags.h
#pragma once
#include "NativeGameplayTags.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_StatsChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Game_StatsChangedDebug);
```

```cpp
// MyNativeTags.cpp
#include "MyNativeTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Game_StatsChanged, "Game.StatsChanged", "Stats changed channel");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Game_StatsChangedDebug, "Game.StatsChangedDebug", "Debug stats channel");
```

### 2) Declare delegate + publisher

```cpp
// StatsComponent.h
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsChanged, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatsChangedDebug, int32, NewValue);

UCLASS()
class UStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FOnStatsChanged OnStatsChanged;

    UPROPERTY(BlueprintAssignable)
    FOnStatsChangedDebug OnStatsChangedDebug;
};
```

### 3) Define event tags (typed)

```cpp
// MyEventTags.h
#pragma once
#include "EventBus/EventTags.h"
#include "MyNativeTags.h"
#include "StatsComponent.h"

NFL_DECLARE_EVENT_TAG(
    FStatsChangedTag,
    FOnStatsChanged,
    UStatsComponent,
    TAG_Game_StatsChanged,
    OnStatsChanged
);

NFL_DECLARE_EVENT_TAG(
    FStatsChangedDebugTag,
    FOnStatsChangedDebug,
    UStatsComponent,
    TAG_Game_StatsChangedDebug,
    OnStatsChangedDebug
);
```

### 4) Add a manager to a subsystem

```cpp
// EventBusSubsystem.h
#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBus/EventBusManager.h"
#include "EventBusSubsystem.generated.h"

UCLASS()
class UEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    Nfrrlib::FEventBusManager& GetManager() { return Manager; }
    virtual void Deinitialize() override { Manager.Reset(); Super::Deinitialize(); }

private:
    Nfrrlib::FEventBusManager Manager;
};
```

### 5) Register publishers (BeginPlay/EndPlay)

```cpp
// StatsComponent.cpp
#include "EventBusSubsystem.h"
#include "MyEventTags.h"

void UStatsComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UEventBusSubsystem* Hub = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        Manager.AddPublisher<FStatsChangedTag>(this);
        Manager.AddPublisher<FStatsChangedDebugTag>(this);
    }
}

void UStatsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UEventBusSubsystem* Hub = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        Manager.RemovePublisher<FStatsChangedTag>(this);
        Manager.RemovePublisher<FStatsChangedDebugTag>(this);
    }

    Super::EndPlay(EndPlayReason);
}
```

### 6) Register listeners

```cpp
// GameMode.cpp (or any listener UObject)
#include "EventBusSubsystem.h"
#include "MyEventTags.h"

UFUNCTION()
void HandleStatsChanged(int32 NewValue);

UFUNCTION()
void HandleStatsChangedDebug(int32 NewValue);

void RegisterListeners(UEventBusSubsystem* Hub)
{
    auto& Manager = Hub->GetManager();

    NFL_EVENTBUS_ADD_LISTENER(Manager, FStatsChangedTag, this, &ThisClass::HandleStatsChanged);
    NFL_EVENTBUS_ADD_LISTENER(Manager, FStatsChangedDebugTag, this, &ThisClass::HandleStatsChangedDebug);
}

void UnregisterListeners(UEventBusSubsystem* Hub)
{
    auto& Manager = Hub->GetManager();

    NFL_EVENTBUS_REMOVE_LISTENER(Manager, FStatsChangedTag, this, &ThisClass::HandleStatsChanged);
    NFL_EVENTBUS_REMOVE_LISTENER(Manager, FStatsChangedDebugTag, this, &ThisClass::HandleStatsChangedDebug);
}
```

---

## Runtime and Blueprint Channels

The manager can route channels dynamically using `FGameplayTag` without a compile-time tag type. This is useful for runtime setups or Blueprint-driven logic.

### Register a runtime publisher

```cpp
Hub->AddPublisherByChannel(
    TAG_Game_StatsChanged,
    PublisherObject,
    GET_MEMBER_NAME_CHECKED(UStatsComponent, OnStatsChanged)
);
```

### Register a runtime listener

```cpp
Hub->AddListenerByChannel(
    TAG_Game_StatsChanged,
    ListenerObject,
    GET_FUNCTION_NAME_CHECKED(UYourListenerClass, HandleStatsChanged)
);
```

### Blueprint usage

In Blueprint, the same subsystem functions are used:

#### AddPublisherByChannel

- `ChannelTag`: gameplay tag for the channel
- `PublisherObj`: target UObject (publisher)
- `DelegatePropertyName`: name of the multicast delegate property

#### AddListenerByChannel

- `ChannelTag`: gameplay tag for the channel
- `ListenerObj`: target UObject (listener)
- `FunctionName`: UFUNCTION name to bind

Removal uses **RemovePublisherByChannel** and **RemoveListenerByChannel**.

---

## Ownership Policy (bOwnsPublisherDelegates)

`TEventBus` supports a policy flag that controls how bindings are removed:

- `bOwnsPublisherDelegates = false` (default)
  - Removes **only** the callback registered by EventBus.
  - Safe when delegates are shared with other systems.

- `bOwnsPublisherDelegates = true`
  - Removes **all** bindings for a listener object (uses `RemoveAll`).
  - Should be used only if the delegate is exclusively owned by EventBus.

---

## Threading

All EventBus APIs are **game-thread only**. Internal guards log and early-out if called from other threads.

---

## Error Handling and Logging

- The log category is `LogNFLEventBus`.
- Invalid pointers or invalid routes emit warnings and return `false` on channel APIs.

---

## Notes on Deterministic Listener IDs

Listener IDs are derived from **object path + function name**, generating deterministic `FGuid` values. This allows removal without storing IDs manually.

---

## Recommended Usage Pattern

- A single `FEventBusManager` should be placed in a `UGameInstanceSubsystem`.
- Publishers should be registered in `BeginPlay` and unregistered in `EndPlay`.
- Listeners should be registered once (or on demand) using typed macros or runtime routes.
- Gameplay tags should be kept unique per channel to avoid route collisions.

---

## Limitations

- Not thread-safe. All calls are expected on the game thread.
- Dynamic delegate binding is not free; heavy high-frequency events should be profiled.
- The system does not provide network replication; replication must be handled separately.

---

## API Summary (Manager)

Typed:

- `AddPublisher<TEventTag>(Publisher)`
- `RemovePublisher<TEventTag>(Publisher)`
- `AddListener<TEventTag>(Listener, FuncName)`
- `RemoveListener<TEventTag>(Listener, FuncName)`

Runtime:

- `AddPublisherByChannel(ChannelTag, Publisher)`
- `AddPublisherByChannel(ChannelTag, Publisher, DelegatePropertyName)`
- `RemovePublisherByChannel(ChannelTag, Publisher)`
- `AddListenerByChannel(ChannelTag, Listener, FuncName)`
- `RemoveListenerByChannel(ChannelTag, Listener, FuncName)`

---

## Packaging

This plugin is runtime-safe and does not depend on editor-only modules. It can be packaged as part of a game build.

---

## System Architecture

```
+-----------------------------------------------------------------+
|                      FEventBusManager                           |
|  (Registry/Factory - typically in GameInstanceSubsystem)        |
+-------------------------------+---------------------------------+
| Manages typed buses           | Manages runtime channels        |
+---------------+---------------+-----------------+---------------+
                |                                 |
        +-------+--------+                +-------+--------+
        | TEventBus<Tag> |                | RuntimeChannel |
        +-------+--------+                +-------+--------+
                |                                 |
        +-------+--------+                +-------+--------+
        | Publisher(s)   |                | Publisher(s)   |
        | Listener(s)    |                | Listener(s)    |
        +----------------+                +----------------+

Communication Flow:
1. Publisher registers -> bus stores weak reference + delegate accessor
2. Listener registers -> bus binds listener to all publishers on channel
3. Publisher broadcasts -> all listeners receive event automatically
4. Cleanup -> weak pointers auto-cleanup destroyed objects
```

### Key Components

- **Manager**: central registry managing all buses and channel routes
- **Bus**: type-safe event routing for a specific channel (compile-time)
- **Channel**: runtime routing using gameplay tag + reflection
- **Publisher**: UObject with multicast delegate (event source)
- **Listener**: UObject with UFUNCTION callback (event receiver)

---

## Complete Working Example

A full end-to-end implementation is provided below to show all components working together.

### 1) Setup EventBus Subsystem

```cpp
// EventBusSubsystem.h
#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBus/EventBusManager.h"
#include "EventBusSubsystem.generated.h"

UCLASS()
class UEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    Nfrrlib::FEventBusManager& GetManager() { return Manager; }

    virtual void Initialize(FSubsystemCollectionBase& Collection) override
    {
        Super::Initialize(Collection);
        UE_LOG(LogTemp, Log, TEXT("EventBus Subsystem Initialized"));
    }

    virtual void Deinitialize() override
    {
        Manager.Reset();
        Super::Deinitialize();
    }

private:
    Nfrrlib::FEventBusManager Manager;
};
```

### 2) Define Native Tags

```cpp
// GameplayTags.h
#pragma once
#include "NativeGameplayTags.h"

// Stats events
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Player_HealthChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Player_StaminaChanged);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Combat_DamageDealt);
```

```cpp
// GameplayTags.cpp
#include "GameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Player_HealthChanged,
    "Event.Player.HealthChanged", "Fired when player health changes");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Player_StaminaChanged,
    "Event.Player.StaminaChanged", "Fired when player stamina changes");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Event_Combat_DamageDealt,
    "Event.Combat.DamageDealt", "Fired when damage is dealt");
```

### 3) Create Publisher Component

```cpp
// StatsComponent.h
#pragma once
#include "Components/ActorComponent.h"
#include "StatsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UStatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UStatsComponent();

    UPROPERTY(BlueprintAssignable)
    FOnHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable)
    FOnStaminaChanged OnStaminaChanged;

    UFUNCTION(BlueprintCallable)
    void SetHealth(float NewHealth);

    UFUNCTION(BlueprintCallable)
    void SetStamina(float NewStamina);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(EditAnywhere, Category = "Stats")
    float MaxHealth = 100.f;

    UPROPERTY(EditAnywhere, Category = "Stats")
    float MaxStamina = 100.f;

    float CurrentHealth = 100.f;
    float CurrentStamina = 100.f;
};
```

```cpp
// StatsComponent.cpp
#include "StatsComponent.h"
#include "EventBusSubsystem.h"
#include "EventTags.h"

void UStatsComponent::BeginPlay()
{
    Super::BeginPlay();

    if (UEventBusSubsystem* Hub = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        Manager.AddPublisher<FHealthChangedTag>(this);
        Manager.AddPublisher<FStaminaChangedTag>(this);
    }
}

void UStatsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UEventBusSubsystem* Hub = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        Manager.RemovePublisher<FHealthChangedTag>(this);
        Manager.RemovePublisher<FStaminaChangedTag>(this);
    }

    Super::EndPlay(EndPlayReason);
}

void UStatsComponent::SetHealth(float NewHealth)
{
    CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UStatsComponent::SetStamina(float NewStamina)
{
    CurrentStamina = FMath::Clamp(NewStamina, 0.f, MaxStamina);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}
```

### 4) Define Event Tags

```cpp
// EventTags.h
#pragma once
#include "EventBus/EventTags.h"
#include "GameplayTags.h"
#include "StatsComponent.h"

NFL_DECLARE_EVENT_TAG(
    FHealthChangedTag,
    FOnHealthChanged,
    UStatsComponent,
    TAG_Event_Player_HealthChanged,
    OnHealthChanged
);

NFL_DECLARE_EVENT_TAG(
    FStaminaChangedTag,
    FOnStaminaChanged,
    UStatsComponent,
    TAG_Event_Player_StaminaChanged,
    OnStaminaChanged
);
```

### 5) Create Listeners (UI + Debug)

```cpp
// HealthBarWidget.h
UCLASS()
class UHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void OnPlayerHealthChanged(float NewHealth, float MaxHealth);

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthBar;
};
```

```cpp
// HealthBarWidget.cpp
#include "HealthBarWidget.h"
#include "EventBusSubsystem.h"
#include "EventTags.h"
#include "Components/ProgressBar.h"

void UHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UEventBusSubsystem* Hub = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        NFL_EVENTBUS_ADD_LISTENER(Manager, FHealthChangedTag, this, &ThisClass::OnPlayerHealthChanged);
    }
}

void UHealthBarWidget::NativeDestruct()
{
    if (UEventBusSubsystem* Hub = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        NFL_EVENTBUS_REMOVE_LISTENER(Manager, FHealthChangedTag, this, &ThisClass::OnPlayerHealthChanged);
    }

    Super::NativeDestruct();
}

void UHealthBarWidget::OnPlayerHealthChanged(float NewHealth, float MaxHealth)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(NewHealth / MaxHealth);
    }
}
```

```cpp
// DebugStatsLogger.h (second listener on same channel)
UCLASS()
class ADebugStatsLogger : public AActor
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void OnHealthChanged(float NewHealth, float MaxHealth);

    UFUNCTION()
    void OnStaminaChanged(float NewStamina, float MaxStamina);
};
```

```cpp
// DebugStatsLogger.cpp
void ADebugStatsLogger::BeginPlay()
{
    Super::BeginPlay();

    if (UEventBusSubsystem* Hub = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto& Manager = Hub->GetManager();
        NFL_EVENTBUS_ADD_LISTENER(Manager, FHealthChangedTag, this, &ThisClass::OnHealthChanged);
        NFL_EVENTBUS_ADD_LISTENER(Manager, FStaminaChangedTag, this, &ThisClass::OnStaminaChanged);
    }
}

void ADebugStatsLogger::OnHealthChanged(float NewHealth, float MaxHealth)
{
    UE_LOG(LogTemp, Warning, TEXT("DEBUG: Health = %.1f / %.1f (%.0f%%)"),
        NewHealth, MaxHealth, (NewHealth / MaxHealth) * 100.f);
}

void ADebugStatsLogger::OnStaminaChanged(float NewStamina, float MaxStamina)
{
    UE_LOG(LogTemp, Warning, TEXT("DEBUG: Stamina = %.1f / %.1f (%.0f%%)"),
        NewStamina, MaxStamina, (NewStamina / MaxStamina) * 100.f);
}
```

---

## Advanced Usage Patterns

### Multiple Channels Per Publisher

A single publisher can expose multiple delegates on different channels:

```cpp
void UMyComponent::BeginPlay()
{
    Super::BeginPlay();

    auto& Manager = GetEventBusManager();
    Manager.AddPublisher<FEventTypeA>(this);
    Manager.AddPublisher<FEventTypeB>(this);
    Manager.AddPublisher<FEventTypeC>(this);
}
```

### Conditional Listening (Dynamic Registration)

Listeners can be registered or removed based on game state:

```cpp
void AMyActor::EnableDebugMode(bool bEnable)
{
    auto& Manager = GetEventBusManager();

    if (bEnable)
    {
        NFL_EVENTBUS_ADD_LISTENER(Manager, FDebugEventTag, this, &ThisClass::OnDebugEvent);
        UE_LOG(LogTemp, Log, TEXT("Debug listener enabled"));
    }
    else
    {
        NFL_EVENTBUS_REMOVE_LISTENER(Manager, FDebugEventTag, this, &ThisClass::OnDebugEvent);
        UE_LOG(LogTemp, Log, TEXT("Debug listener disabled"));
    }
}
```

### Event Filtering in Listeners

Custom filtering logic can be implemented in listener callbacks:

```cpp
UFUNCTION()
void OnDamageDealt(AActor* Victim, float Damage)
{
    if (Damage < 50.f)
    {
        return;
    }

    if (!Victim->IsA<AEnemy>())
    {
        return;
    }

    HandleHighDamageToEnemy(Victim, Damage);
}
```

### Cascading Events

One listener can trigger other events:

```cpp
UFUNCTION()
void OnHealthChanged(float NewHealth, float MaxHealth)
{
    if (NewHealth / MaxHealth < 0.25f)
    {
        OnLowHealthWarning.Broadcast();
    }
}
```

### Runtime Channel Discovery (Planned)

A public enumeration API for channel routes is planned for a future release. At present, there is no public channel iteration API.

---

## Performance Considerations

### Best Practices

Recommended: register once, use many times.

```cpp
// GOOD: Register in BeginPlay
void BeginPlay()
{
    Manager.AddPublisher<FMyEventTag>(this);
}

// BAD: Per-frame registration is heavy
void Tick(float DeltaTime)
{
    Manager.AddPublisher<FMyEventTag>(this);
}
```

Recommended: use for gameplay-frequency events.

- Stats changes (health, mana, stamina)
- Ability activations
- Quest updates
- Achievement unlocks
- UI notifications

Avoid: high-frequency events.

- Per-tick updates (use direct calls or delegates)
- Physics callbacks
- Animation notifies (unless filtered)
- Input polling

### Performance Characteristics

| Operation | Cost | Notes |
| --------- | ---- |------ |
| Register Publisher | Medium | Binds to all existing listeners |
| Register Listener | Medium | Binds to all existing publishers |
| Broadcast Event | Low | Standard delegate call |
| Unregister | Low | Cleanup is efficient |
| Stale Object Cleanup | Low | Lazy, only on next operation |

### Profiling

Unreal profiling tools can be used to monitor delegate overhead:

```cpp
// Console commands
stat ScriptDelegates  // Monitor delegate performance
stat Game             // General gameplay stats
```

### Memory Usage

- **Per Bus**: ~200 bytes overhead
- **Per Publisher**: ~100 bytes + weak pointer
- **Per Listener**: ~150 bytes + script delegate
- **Cleanup**: automatic via weak pointers (no manual memory management)

---

## Troubleshooting

### Common Issues

#### Issue: Listener not receiving events

Checklist:

- Channel tag matches exactly between publisher and listener
- Both publisher and listener registered before broadcast
- UFUNCTION signature matches delegate exactly (parameter types, order, names)
- Objects are valid (not pending kill or destroyed)
- Function marked as `UFUNCTION()` with correct specifiers

Debug:

```cpp
// Enable verbose logging
UE_LOG(LogNFLEventBus, Log, TEXT("Publisher registered: %s"), *GetNameSafe(Publisher));
```

#### Issue: Compile errors with concepts

Solution:

```cpp
// <Project>.Target.cs
public class ProjectTarget : TargetRules
{
    public ProjectTarget(TargetInfo Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
    }
}
```

#### Issue: Events stop firing after object destruction

This is expected behavior:

- EventBus uses `TWeakObjectPtr` for automatic cleanup
- Destroyed publishers/listeners are cleaned lazily
- No manual cleanup is required
- Prevents dangling pointer crashes

#### Issue: Delegate signature mismatch error

```cpp
// ERROR: Delegate expects (int32), listener provides (float)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEvent, int32, Value);

UFUNCTION()
void OnEvent(float Value); // Wrong type

// FIX: Match types exactly
UFUNCTION()
void OnEvent(int32 Value); // Correct
```

#### Issue: Publisher/Listener in different modules

The EventBus module should be included in both module dependencies:

```cpp
// PublisherModule.Build.cs
PublicDependencyModuleNames.AddRange(new string[] { "EventBus", "GameplayTags" });

// ListenerModule.Build.cs
PublicDependencyModuleNames.AddRange(new string[] { "EventBus", "GameplayTags" });
```

---

## Integration with Existing Code

### Replacing Hard References

#### Before (tight coupling)

```cpp
// PlayerCharacter.h
UPROPERTY()
UHealthBarWidget* HealthBar; // Hard reference

void TakeDamage(float Damage)
{
    Health -= Damage;
    if (HealthBar)
    {
        HealthBar->UpdateHealth(Health); // Direct call
    }
}
```

#### After (loose coupling via EventBus)

```cpp
// PlayerCharacter.h
UPROPERTY(BlueprintAssignable)
FOnHealthChanged OnHealthChanged;

void TakeDamage(float Damage)
{
    Health -= Damage;
    OnHealthChanged.Broadcast(Health, MaxHealth); // Any listener receives
}

// HealthBarWidget.cpp
void UHealthBarWidget::NativeConstruct()
{
    NFL_EVENTBUS_ADD_LISTENER(Manager, FHealthChangedTag, this, &ThisClass::UpdateHealth);
}
```

### Modular Architecture

EventBus can be used for cross-module communication without creating direct module dependencies:

```
+-------------+        EventBus        +--------------+
| CombatModule| ------broadcast------> |  UIModule    |
| (Publisher) |                        | (Listener)   |
+-------------+                        +--------------+
      |                                        |
      +-------------- No Direct Dependency ----+
```

### Blueprint Integration

A Blueprint Function Library can be created to wrap subsystem access:

```cpp
UCLASS()
class UEventBusLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
    static void RegisterPublisher(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* Publisher, FName DelegateName);

    UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
    static void RegisterListener(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* Listener, FName FunctionName);
};
```

---

## FAQ

### Q: Is EventBus suitable for network replication?

A: No. EventBus is local-only. Standard UE replication (`DOREPLIFETIME`, RPCs) is required for network events.

### Q: Is EventBus thread-safe?

A: No. All APIs must be called on the game thread. Internal guards prevent misuse.

### Q: Can multiple publishers share a channel?

A: Yes. EventBus supports multiple publishers on the same channel. All listeners receive events from all publishers.

### Q: What is the overhead compared to direct delegates?

A: Minimal. EventBus adds one level of indirection (weak pointer lookup) while preventing tight coupling.

### Q: Is C++17 supported?

A: The core API requires C++20. `Util.h` provides a C++17 fallback only for validity helpers.

### Q: Is Blueprint-only usage supported?

A: Runtime channels can be used via Blueprint. Typed tags require C++.

---

## License

This plugin is provided as-is for educational and commercial use. Modifications should be applied as needed by the project.

---

## Credits

**Author**: Niccolo Ferrari (<https://github.com/NickF93>)
**Repository**: <https://github.com/DBGA-Master-Online-Novembre-2025/DungeonBreakerGloryAwaits>  
**Unreal Engine**: 5.5  
**C++ Standard**: C++20
