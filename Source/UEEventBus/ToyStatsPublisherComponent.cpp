#include "ToyStatsPublisherComponent.h"

#include "Engine/GameInstance.h"

#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Typed/EventChannelApi.h"

#include "EventBusToyChannels.h"

UToyStatsPublisherComponent::UToyStatsPublisherComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UToyStatsPublisherComponent::SetHealth(const float InHealth)
{
	Health = InHealth;
	OnToyHealthChanged.Broadcast(Health);
}

void UToyStatsPublisherComponent::SetStamina(const float InStamina)
{
	Stamina = InStamina;
	OnToyStaminaChanged.Broadcast(Stamina);
}

void UToyStatsPublisherComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!::IsValid(World))
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!::IsValid(GameInstance))
	{
		return;
	}

	UEventBusSubsystem* EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
	if (!::IsValid(EventBusSubsystem))
	{
		return;
	}

	Nfrrlib::EventBus::FEventBus& Bus = EventBusSubsystem->GetEventBus();
	Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::Register(Bus, false);
	Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::Register(Bus, false);

	Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::AddPublisher(Bus, this);
	Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::AddPublisher(Bus, this);
}

void UToyStatsPublisherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	if (::IsValid(World))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UEventBusSubsystem* EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
			{
				Nfrrlib::EventBus::FEventBus& Bus = EventBusSubsystem->GetEventBus();
				Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::RemovePublisher(Bus, this);
				Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::RemovePublisher(Bus, this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

