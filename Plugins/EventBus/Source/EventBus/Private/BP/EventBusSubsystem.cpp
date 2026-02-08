#include "EventBus/BP/EventBusSubsystem.h"

#include "Subsystems/SubsystemCollection.h"

#include "EventBus/BP/EventBusRegistryAsset.h"

#include "EventBus/Core/EventBus.h"

/**
 * @brief Emits subsystem startup diagnostics for runtime tracing.
 */
void UEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogNFLEventBus, Log,
		TEXT("EventBusSubsystem::Initialize. GameInstance=%s InitialRegistry=%s"),
		*GetNameSafe(GetGameInstance()),
		*GetNameSafe(Registry.Get()));
}

/**
 * @brief Performs deterministic EventBus teardown during subsystem shutdown.
 */
void UEventBusSubsystem::Deinitialize()
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("EventBusSubsystem::Deinitialize. GameInstance=%s ActiveRegistry=%s"),
		*GetNameSafe(GetGameInstance()),
		*GetNameSafe(Registry.Get()));

	EventBus.Reset();
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
 * @brief Returns active registry asset used for blueprint allowlist checks.
 */
const UEventBusRegistryAsset* UEventBusSubsystem::GetRegistry() const
{
	return Registry.Get();
}

/**
 * @brief Assigns registry asset used by blueprint validated operations.
 */
void UEventBusSubsystem::SetRegistry(UEventBusRegistryAsset* InRegistry)
{
	UE_LOG(LogNFLEventBus, Display,
		TEXT("EventBusSubsystem::SetRegistry. Previous=%s New=%s"),
		*GetNameSafe(Registry.Get()),
		*GetNameSafe(InRegistry));

	Registry = InRegistry;
}
