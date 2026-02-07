#include "ToyCppListenerActor.h"

#include "Engine/GameInstance.h"

#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Typed/EventChannelApi.h"

#include "EventBusToyChannels.h"

/**
 * @brief Registers toy listener callbacks into typed channels at actor startup.
 */
void AToyCppListenerActor::BeginPlay()
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
	const bool bHealthListenerAdded = NFL_EVENTBUS_ADD_LISTENER(Bus, FToyHealthChangedChannel, this, ThisClass, OnHealthChanged);
	const bool bStaminaListenerAdded = NFL_EVENTBUS_ADD_LISTENER(Bus, FToyStaminaChangedChannel, this, ThisClass, OnStaminaChanged);
	if (!bHealthListenerAdded || !bStaminaListenerAdded)
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Toy listener registration incomplete. HealthAdded=%d StaminaAdded=%d"),
			bHealthListenerAdded,
			bStaminaListenerAdded);
	}
}

/**
 * @brief Removes toy listener callbacks from typed channels during actor teardown.
 */
void AToyCppListenerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWorld* World = GetWorld();
	if (::IsValid(World))
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UEventBusSubsystem* EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
			{
				Nfrrlib::EventBus::FEventBus& Bus = EventBusSubsystem->GetEventBus();
				const bool bHealthListenerRemoved = NFL_EVENTBUS_REMOVE_LISTENER(Bus, FToyHealthChangedChannel, this, ThisClass, OnHealthChanged);
				const bool bStaminaListenerRemoved = NFL_EVENTBUS_REMOVE_LISTENER(Bus, FToyStaminaChangedChannel, this, ThisClass, OnStaminaChanged);
				if (!bHealthListenerRemoved || !bStaminaListenerRemoved)
				{
					UE_LOG(LogNFLEventBus, Warning,
						TEXT("Toy listener teardown incomplete. HealthRemoved=%d StaminaRemoved=%d"),
						bHealthListenerRemoved,
						bStaminaListenerRemoved);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

/**
 * @brief Toy health callback implementation.
 */
void AToyCppListenerActor::OnHealthChanged(const float NewHealth)
{
	UE_LOG(LogNFLEventBus, Log, TEXT("Toy C++ Listener: Health changed = %.2f"), NewHealth);
}

/**
 * @brief Toy stamina callback implementation.
 */
void AToyCppListenerActor::OnStaminaChanged(const float NewStamina)
{
	UE_LOG(LogNFLEventBus, Log, TEXT("Toy C++ Listener: Stamina changed = %.2f"), NewStamina);
}
