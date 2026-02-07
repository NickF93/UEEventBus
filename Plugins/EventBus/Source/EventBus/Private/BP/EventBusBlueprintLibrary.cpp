#include "EventBus/BP/EventBusBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"

#include "EventBus/BP/EventBusRegistryAsset.h"
#include "EventBus/BP/EventBusSubsystem.h"
#include "EventBus/Core/EventBus.h"

namespace
{
	/**
	 * @brief Shared subsystem lookup helper used by all Blueprint runtime entry points.
	 */
	FORCEINLINE UEventBusSubsystem* ResolveEventBusSubsystem(UObject* WorldContextObject)
	{
		if (!::IsValid(WorldContextObject) || !GEngine)
		{
			return nullptr;
		}

		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		if (!::IsValid(World))
		{
			return nullptr;
		}

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!::IsValid(GameInstance))
		{
			return nullptr;
		}

		return GameInstance->GetSubsystem<UEventBusSubsystem>();
	}
}

bool UEventBusBlueprintLibrary::RegisterChannel(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	const bool bOwnsPublisherDelegates)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		return false;
	}

	Nfrrlib::EventBus::FChannelRegistration Registration;
	Registration.ChannelTag = ChannelTag;
	Registration.bOwnsPublisherDelegates = bOwnsPublisherDelegates;
	return Subsystem->GetEventBus().RegisterChannel(Registration);
}

/**
 * @brief Blueprint facade wrapper for channel unregistration.
 */
bool UEventBusBlueprintLibrary::UnregisterChannel(UObject* WorldContextObject, const FGameplayTag ChannelTag)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		return false;
	}

	return Subsystem->GetEventBus().UnregisterChannel(ChannelTag);
}

/**
 * @brief Blueprint facade wrapper for allowlisted publisher registration.
 */
bool UEventBusBlueprintLibrary::AddPublisherValidated(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* PublisherObj,
	const FName DelegatePropertyName)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem) || !::IsValid(PublisherObj))
	{
		return false;
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	if (!::IsValid(Registry) || !Registry->IsPublisherAllowed(ChannelTag, PublisherObj->GetClass(), DelegatePropertyName))
	{
		UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherValidated denied. Channel=%s Publisher=%s Delegate=%s"),
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherObj),
			*DelegatePropertyName.ToString());
		return false;
	}

	Nfrrlib::EventBus::FPublisherBinding Binding;
	Binding.DelegatePropertyName = DelegatePropertyName;
	return Subsystem->GetEventBus().AddPublisher(ChannelTag, PublisherObj, Binding);
}

/**
 * @brief Blueprint facade wrapper for publisher removal.
 */
bool UEventBusBlueprintLibrary::RemovePublisher(UObject* WorldContextObject, const FGameplayTag ChannelTag, UObject* PublisherObj)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		return false;
	}

	return Subsystem->GetEventBus().RemovePublisher(ChannelTag, PublisherObj);
}

/**
 * @brief Blueprint facade wrapper for allowlisted listener registration.
 */
bool UEventBusBlueprintLibrary::AddListenerValidated(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* ListenerObj,
	const FName FunctionName)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem) || !::IsValid(ListenerObj))
	{
		return false;
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	if (!::IsValid(Registry) || !Registry->IsListenerAllowed(ChannelTag, ListenerObj->GetClass(), FunctionName))
	{
		UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerValidated denied. Channel=%s Listener=%s Func=%s"),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerObj),
			*FunctionName.ToString());
		return false;
	}

	Nfrrlib::EventBus::FListenerBinding Binding;
	Binding.FunctionName = FunctionName;
	return Subsystem->GetEventBus().AddListener(ChannelTag, ListenerObj, Binding);
}

/**
 * @brief Blueprint facade wrapper for listener removal.
 */
bool UEventBusBlueprintLibrary::RemoveListener(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* ListenerObj,
	const FName FunctionName)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		return false;
	}

	Nfrrlib::EventBus::FListenerBinding Binding;
	Binding.FunctionName = FunctionName;
	return Subsystem->GetEventBus().RemoveListener(ChannelTag, ListenerObj, Binding);
}

/**
 * @brief Returns sorted, deduplicated allowlisted listener functions for a class on a channel.
 */
TArray<FName> UEventBusBlueprintLibrary::GetAllowedListenerFunctions(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	const TSubclassOf<UObject> ListenerClass)
{
	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		return {};
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	return ::IsValid(Registry) ? Registry->GetAllowedListenerFunctions(ChannelTag, ListenerClass.Get()) : TArray<FName>();
}

/**
 * @brief Centralized subsystem resolve wrapper for all blueprint entry points.
 */
UEventBusSubsystem* UEventBusBlueprintLibrary::ResolveSubsystem(UObject* WorldContextObject)
{
	return ResolveEventBusSubsystem(WorldContextObject);
}
