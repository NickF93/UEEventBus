#include "EventBus/BP/EventBusSubsystem.h"

#include "Subsystems/SubsystemCollection.h"

#include "EventBus/BP/EventBusRegistryAsset.h"

/**
 * @brief Emits subsystem startup diagnostics for runtime tracing.
 */
void UEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!::IsValid(RuntimeRegistry.Get()))
	{
		RuntimeRegistry = NewObject<UEventBusRegistryAsset>(
			this,
			UEventBusRegistryAsset::StaticClass(),
			NAME_None,
			RF_Transient);
	}

	UE_LOG(LogNFLEventBus, Log,
		TEXT("EventBusSubsystem::Initialize. GameInstance=%s RuntimeRegistry=%s"),
		*GetNameSafe(GetGameInstance()),
		*GetNameSafe(RuntimeRegistry.Get()));
}

/**
 * @brief Performs deterministic EventBus teardown during subsystem shutdown.
 */
void UEventBusSubsystem::Deinitialize()
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("EventBusSubsystem::Deinitialize. GameInstance=%s RuntimeRegistry=%s"),
		*GetNameSafe(GetGameInstance()),
		*GetNameSafe(RuntimeRegistry.Get()));

	EventBus.Reset();
	RuntimeRegistry = nullptr;
	Super::Deinitialize();
}

/**
 * @brief Returns mutable EventBus runtime owned by this subsystem.
 */
Nfrrlib::EventBus::FEventBus& UEventBusSubsystem::GetEventBus()
{
	return EventBus;
}

/**
 * @brief Returns active runtime registry used for blueprint bind history.
 */
const UEventBusRegistryAsset* UEventBusSubsystem::GetRuntimeRegistry() const
{
	return RuntimeRegistry.Get();
}
