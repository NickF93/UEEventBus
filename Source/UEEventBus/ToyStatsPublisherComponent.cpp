#include "ToyStatsPublisherComponent.h"

#include "Engine/GameInstance.h"

#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Typed/EventChannelApi.h"

#include "EventBusToyChannels.h"

/**
 * @brief Creates non-ticking toy publisher component.
 */
UToyStatsPublisherComponent::UToyStatsPublisherComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

/**
 * @brief Updates health state and broadcasts EventBus-backed health delegate.
 */
void UToyStatsPublisherComponent::SetHealth(const float InHealth)
{
	Health = InHealth;
	OnToyHealthChanged.Broadcast(Health);
}

/**
 * @brief Updates stamina state and broadcasts EventBus-backed stamina delegate.
 */
void UToyStatsPublisherComponent::SetStamina(const float InStamina)
{
	Stamina = InStamina;
	OnToyStaminaChanged.Broadcast(Stamina);
}

/**
 * @brief Registers toy channels and publisher bindings at component startup.
 */
void UToyStatsPublisherComponent::BeginPlay()
{
	Super::BeginPlay();

	UWorld* const World = GetWorld();
	if (!::IsValid(World))
	{
		return;
	}

	UGameInstance* const GameInstance = World->GetGameInstance();
	if (!::IsValid(GameInstance))
	{
		return;
	}

	UEventBusSubsystem* const EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
	if (!::IsValid(EventBusSubsystem))
	{
		return;
	}

	Nfrrlib::EventBus::FEventBus& Bus = EventBusSubsystem->GetEventBus();
	const bool bHealthRegistered = Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::Register(Bus, false);
	const bool bStaminaRegistered = Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::Register(Bus, false);
	const bool bHealthPublisherAdded = Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::AddPublisher(Bus, this);
	const bool bStaminaPublisherAdded = Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::AddPublisher(Bus, this);

	if (!bHealthRegistered || !bStaminaRegistered || !bHealthPublisherAdded || !bStaminaPublisherAdded)
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Toy publisher registration incomplete. HealthReg=%d StaminaReg=%d HealthPub=%d StaminaPub=%d"),
			bHealthRegistered,
			bStaminaRegistered,
			bHealthPublisherAdded,
			bStaminaPublisherAdded);
	}
}

/**
 * @brief Removes toy publisher bindings during component teardown.
 */
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
				const bool bHealthPublisherRemoved = Nfrrlib::EventBus::TEventChannelApi<FToyHealthChangedChannel>::RemovePublisher(Bus, this);
				const bool bStaminaPublisherRemoved = Nfrrlib::EventBus::TEventChannelApi<FToyStaminaChangedChannel>::RemovePublisher(Bus, this);
				if (!bHealthPublisherRemoved || !bStaminaPublisherRemoved)
				{
					UE_LOG(LogNFLEventBus, Warning,
						TEXT("Toy publisher teardown incomplete. HealthRemoved=%d StaminaRemoved=%d"),
						bHealthPublisherRemoved,
						bStaminaPublisherRemoved);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}
