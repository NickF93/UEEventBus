#include "ToyEventBusChannelsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Subsystems/SubsystemCollection.h"

#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Core/EventBus.h"
#include "EventBus/Typed/EventChannelApi.h"

#include "EventBusToyChannels.h"

/**
 * @brief Registers toy channels once per game instance so listeners can bind before publishers begin play.
 */
void UToyEventBusChannelsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UEventBusSubsystem>();

	UGameInstance* const GameInstance = GetGameInstance();
	if (!::IsValid(GameInstance))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("ToyEventBusChannelsSubsystem::Initialize failed. GameInstance is invalid."));
		return;
	}

	UEventBusSubsystem* const EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
	if (!::IsValid(EventBusSubsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("ToyEventBusChannelsSubsystem::Initialize failed. EventBusSubsystem is unavailable."));
		return;
	}

	Nfrrlib::EventBus::FEventBus& Bus = EventBusSubsystem->GetEventBus();
	const bool bHealthRegistered = Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::Register(Bus, false);
	const bool bStaminaRegistered = Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::Register(Bus, false);

	if (!bHealthRegistered || !bStaminaRegistered)
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("ToyEventBusChannelsSubsystem::Initialize incomplete. HealthReg=%d StaminaReg=%d"),
			bHealthRegistered,
			bStaminaRegistered);
		return;
	}

	UE_LOG(LogNFLEventBus, Log,
		TEXT("ToyEventBusChannelsSubsystem::Initialize completed. HealthReg=%d StaminaReg=%d"),
		bHealthRegistered,
		bStaminaRegistered);
}

