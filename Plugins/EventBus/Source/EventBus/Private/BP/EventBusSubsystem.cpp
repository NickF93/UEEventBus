#include "EventBus/BP/EventBusSubsystem.h"

#include "EventBus/BP/EventBusRegistryAsset.h"

#include "EventBus/Core/EventBus.h"

/**
 * @brief Performs deterministic EventBus teardown during subsystem shutdown.
 */
void UEventBusSubsystem::Deinitialize()
{
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
	Registry = InRegistry;
}
