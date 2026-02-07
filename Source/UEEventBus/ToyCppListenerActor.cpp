#include "ToyCppListenerActor.h"

#include "Engine/GameInstance.h"

#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Typed/EventChannelApi.h"

#include "EventBusToyChannels.h"

void AToyCppListenerActor::BeginPlay()
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
	NFL_EVENTBUS_ADD_LISTENER(Bus, FToyHealthChangedChannel, this, ThisClass, OnHealthChanged);
	NFL_EVENTBUS_ADD_LISTENER(Bus, FToyStaminaChangedChannel, this, ThisClass, OnStaminaChanged);
}

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
				NFL_EVENTBUS_REMOVE_LISTENER(Bus, FToyHealthChangedChannel, this, ThisClass, OnHealthChanged);
				NFL_EVENTBUS_REMOVE_LISTENER(Bus, FToyStaminaChangedChannel, this, ThisClass, OnStaminaChanged);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AToyCppListenerActor::OnHealthChanged(const float NewHealth)
{
	UE_LOG(LogNFLEventBus, Log, TEXT("Toy C++ Listener: Health changed = %.2f"), NewHealth);
}

void AToyCppListenerActor::OnStaminaChanged(const float NewStamina)
{
	UE_LOG(LogNFLEventBus, Log, TEXT("Toy C++ Listener: Stamina changed = %.2f"), NewStamina);
}
