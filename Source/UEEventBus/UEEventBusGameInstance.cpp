#include "UEEventBusGameInstance.h"

#include "EventBus/BP/EventBusRegistryAsset.h"
#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Core/EventBus.h"

/**
 * @brief Sets up EventBus subsystem ownership for this game instance.
 */
void UUEEventBusGameInstance::Init()
{
	Super::Init();

	UEventBusSubsystem* const EventBusSubsystem = GetSubsystem<UEventBusSubsystem>();
	if (!::IsValid(EventBusSubsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("UEEventBusGameInstance::Init failed. EventBusSubsystem is unavailable."));
		return;
	}

	UE_LOG(LogNFLEventBus, Log,
		TEXT("UEEventBusGameInstance::Init completed. RuntimeRegistry=%s"),
		*GetNameSafe(EventBusSubsystem->GetRuntimeRegistry()));
}
