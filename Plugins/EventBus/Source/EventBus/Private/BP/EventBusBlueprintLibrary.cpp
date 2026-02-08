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
		if (!::IsValid(WorldContextObject))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("ResolveEventBusSubsystem failed: WorldContextObject is invalid."));
			return nullptr;
		}

		if (!GEngine)
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("ResolveEventBusSubsystem failed: GEngine is null."));
			return nullptr;
		}

		UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		if (!::IsValid(World))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("ResolveEventBusSubsystem failed: world could not be resolved from context '%s'."),
				*GetNameSafe(WorldContextObject));
			return nullptr;
		}

		UGameInstance* const GameInstance = World->GetGameInstance();
		if (!::IsValid(GameInstance))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("ResolveEventBusSubsystem failed: GameInstance is invalid for world '%s'."),
				*GetNameSafe(World));
			return nullptr;
		}

		UEventBusSubsystem* const Subsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
		if (!::IsValid(Subsystem))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("ResolveEventBusSubsystem failed: EventBusSubsystem is unavailable for GameInstance '%s'."),
				*GetNameSafe(GameInstance));
			return nullptr;
		}

		return Subsystem;
	}
}

/**
 * @brief Blueprint facade wrapper for channel registration.
 */
bool UEventBusBlueprintLibrary::RegisterChannel(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	const bool bOwnsPublisherDelegates)
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RegisterChannel request. Channel=%s bOwnsPublisherDelegates=%s"),
		*ChannelTag.ToString(),
		bOwnsPublisherDelegates ? TEXT("true") : TEXT("false"));

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP RegisterChannel denied: subsystem resolution failed."));
		return false;
	}

	Nfrrlib::EventBus::FChannelRegistration Registration;
	Registration.ChannelTag = ChannelTag;
	Registration.bOwnsPublisherDelegates = bOwnsPublisherDelegates;
	const bool bResult = Subsystem->GetEventBus().RegisterChannel(Registration);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RegisterChannel result. Channel=%s Success=%s"),
		*ChannelTag.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

/**
 * @brief Blueprint facade wrapper for channel unregistration.
 */
bool UEventBusBlueprintLibrary::UnregisterChannel(UObject* WorldContextObject, const FGameplayTag ChannelTag)
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP UnregisterChannel request. Channel=%s"),
		*ChannelTag.ToString());

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP UnregisterChannel denied: subsystem resolution failed."));
		return false;
	}

	const bool bResult = Subsystem->GetEventBus().UnregisterChannel(ChannelTag);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP UnregisterChannel result. Channel=%s Success=%s"),
		*ChannelTag.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
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
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP AddPublisherValidated request. Channel=%s Publisher=%s Delegate=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherObj),
		*DelegatePropertyName.ToString());

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP AddPublisherValidated denied: subsystem resolution failed."));
		return false;
	}

	if (!::IsValid(PublisherObj))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP AddPublisherValidated denied: PublisherObj is invalid."));
		return false;
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	if (!::IsValid(Registry))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("AddPublisherValidated denied: registry is null. Channel=%s Publisher=%s Delegate=%s"),
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherObj),
			*DelegatePropertyName.ToString());
		return false;
	}

	if (!Registry->IsPublisherAllowed(ChannelTag, PublisherObj->GetClass(), DelegatePropertyName))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("AddPublisherValidated denied: no allowlist match. Registry=%s Channel=%s PublisherClass=%s Delegate=%s"),
			*GetNameSafe(Registry),
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherObj->GetClass()),
			*DelegatePropertyName.ToString());
		return false;
	}

	Nfrrlib::EventBus::FPublisherBinding Binding;
	Binding.DelegatePropertyName = DelegatePropertyName;
	const bool bResult = Subsystem->GetEventBus().AddPublisher(ChannelTag, PublisherObj, Binding);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP AddPublisherValidated result. Channel=%s Publisher=%s Delegate=%s Success=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherObj),
		*DelegatePropertyName.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

/**
 * @brief Blueprint facade wrapper for publisher removal.
 */
bool UEventBusBlueprintLibrary::RemovePublisher(UObject* WorldContextObject, const FGameplayTag ChannelTag, UObject* PublisherObj)
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RemovePublisher request. Channel=%s Publisher=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherObj));

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP RemovePublisher denied: subsystem resolution failed."));
		return false;
	}

	const bool bResult = Subsystem->GetEventBus().RemovePublisher(ChannelTag, PublisherObj);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RemovePublisher result. Channel=%s Publisher=%s Success=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherObj),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
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
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP AddListenerValidated request. Channel=%s Listener=%s Function=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerObj),
		*FunctionName.ToString());

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP AddListenerValidated denied: subsystem resolution failed."));
		return false;
	}

	if (!::IsValid(ListenerObj))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP AddListenerValidated denied: ListenerObj is invalid."));
		return false;
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	if (!::IsValid(Registry))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("AddListenerValidated denied: registry is null. Channel=%s Listener=%s Func=%s"),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerObj),
			*FunctionName.ToString());
		return false;
	}

	if (!Registry->IsListenerAllowed(ChannelTag, ListenerObj->GetClass(), FunctionName))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("AddListenerValidated denied: no allowlist match. Registry=%s Channel=%s ListenerClass=%s Func=%s"),
			*GetNameSafe(Registry),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerObj->GetClass()),
			*FunctionName.ToString());
		return false;
	}

	Nfrrlib::EventBus::FListenerBinding Binding;
	Binding.FunctionName = FunctionName;
	const bool bResult = Subsystem->GetEventBus().AddListener(ChannelTag, ListenerObj, Binding);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP AddListenerValidated result. Channel=%s Listener=%s Function=%s Success=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerObj),
		*FunctionName.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
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
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RemoveListener request. Channel=%s Listener=%s Function=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerObj),
		*FunctionName.ToString());

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP RemoveListener denied: subsystem resolution failed."));
		return false;
	}

	Nfrrlib::EventBus::FListenerBinding Binding;
	Binding.FunctionName = FunctionName;
	const bool bResult = Subsystem->GetEventBus().RemoveListener(ChannelTag, ListenerObj, Binding);
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP RemoveListener result. Channel=%s Listener=%s Function=%s Success=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerObj),
		*FunctionName.ToString(),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

/**
 * @brief Returns sorted, deduplicated allowlisted listener functions for a class on a channel.
 */
TArray<FName> UEventBusBlueprintLibrary::GetAllowedListenerFunctions(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	const TSubclassOf<UObject> ListenerClass)
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP GetAllowedListenerFunctions request. Channel=%s ListenerClass=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass.Get()));

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP GetAllowedListenerFunctions denied: subsystem resolution failed."));
		return {};
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRegistry();
	if (!::IsValid(Registry))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP GetAllowedListenerFunctions denied: registry is null."));
		return {};
	}

	TArray<FName> AllowedFunctions = Registry->GetAllowedListenerFunctions(ChannelTag, ListenerClass.Get());
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP GetAllowedListenerFunctions result. Channel=%s ListenerClass=%s Count=%d"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass.Get()),
		AllowedFunctions.Num());
	return AllowedFunctions;
}

/**
 * @brief Centralized subsystem resolve wrapper for all blueprint entry points.
 */
UEventBusSubsystem* UEventBusBlueprintLibrary::ResolveSubsystem(UObject* WorldContextObject)
{
	return ResolveEventBusSubsystem(WorldContextObject);
}
