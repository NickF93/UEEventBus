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

	/**
	 * @brief Records one publisher binding in runtime history registry if available.
	 */
	FORCEINLINE void RecordPublisherHistory(
		UEventBusSubsystem* Subsystem,
		const FGameplayTag& ChannelTag,
		UObject* PublisherObj,
		const FName DelegatePropertyName)
	{
		if (!::IsValid(Subsystem) || !::IsValid(PublisherObj))
		{
			return;
		}

		UEventBusRegistryAsset* Registry = const_cast<UEventBusRegistryAsset*>(Subsystem->GetRuntimeRegistry());
		if (::IsValid(Registry))
		{
			Registry->RecordPublisherBinding(ChannelTag, PublisherObj->GetClass(), DelegatePropertyName);
		}
	}

	/**
	 * @brief Records one listener binding in runtime history registry if available.
	 */
	FORCEINLINE void RecordListenerHistory(
		UEventBusSubsystem* Subsystem,
		const FGameplayTag& ChannelTag,
		UObject* ListenerObj,
		const FName FunctionName)
	{
		if (!::IsValid(Subsystem) || !::IsValid(ListenerObj))
		{
			return;
		}

		UEventBusRegistryAsset* Registry = const_cast<UEventBusRegistryAsset*>(Subsystem->GetRuntimeRegistry());
		if (::IsValid(Registry))
		{
			Registry->RecordListenerBinding(ChannelTag, ListenerObj->GetClass(), FunctionName);
		}
	}

	/**
	 * @brief Validates channel and object inputs shared by BP binding entry points.
	 */
	FORCEINLINE bool ValidateBindingInputs(
		const TCHAR* ApiName,
		const TCHAR* ObjectLabel,
		const FGameplayTag& ChannelTag,
		UObject* BoundObject)
	{
		if (!ChannelTag.IsValid())
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("BP %s denied: ChannelTag is invalid."),
				ApiName);
			return false;
		}

		if (!::IsValid(BoundObject))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("BP %s denied: %s is invalid."),
				ApiName,
				ObjectLabel);
			return false;
		}

		return true;
	}

	/**
	 * @brief Shared implementation for publisher-add APIs to keep behavior and logging aligned.
	 */
	FORCEINLINE bool AddPublisherInternal(
		UObject* WorldContextObject,
		const FGameplayTag& ChannelTag,
		UObject* PublisherObj,
		const FName DelegatePropertyName,
		const TCHAR* ApiName)
	{
		UE_LOG(LogNFLEventBus, Log,
			TEXT("BP %s request. Channel=%s Publisher=%s Delegate=%s"),
			ApiName,
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherObj),
			*DelegatePropertyName.ToString());

		UEventBusSubsystem* const Subsystem = ResolveEventBusSubsystem(WorldContextObject);
		if (!::IsValid(Subsystem))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("BP %s denied: subsystem resolution failed."),
				ApiName);
			return false;
		}

		if (!ValidateBindingInputs(ApiName, TEXT("PublisherObj"), ChannelTag, PublisherObj))
		{
			return false;
		}

		Nfrrlib::EventBus::FPublisherBinding Binding;
		Binding.DelegatePropertyName = DelegatePropertyName;

		const bool bResult = Subsystem->GetEventBus().AddPublisher(ChannelTag, PublisherObj, Binding);
		if (bResult)
		{
			RecordPublisherHistory(Subsystem, ChannelTag, PublisherObj, DelegatePropertyName);
		}

		UE_LOG(LogNFLEventBus, Log,
			TEXT("BP %s result. Channel=%s Publisher=%s Delegate=%s Success=%s"),
			ApiName,
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherObj),
			*DelegatePropertyName.ToString(),
			bResult ? TEXT("true") : TEXT("false"));
		return bResult;
	}

	/**
	 * @brief Shared implementation for listener-add APIs to keep behavior and logging aligned.
	 */
	FORCEINLINE bool AddListenerInternal(
		UObject* WorldContextObject,
		const FGameplayTag& ChannelTag,
		UObject* ListenerObj,
		const FName FunctionName,
		const TCHAR* ApiName)
	{
		UE_LOG(LogNFLEventBus, Log,
			TEXT("BP %s request. Channel=%s Listener=%s Function=%s"),
			ApiName,
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerObj),
			*FunctionName.ToString());

		UEventBusSubsystem* const Subsystem = ResolveEventBusSubsystem(WorldContextObject);
		if (!::IsValid(Subsystem))
		{
			UE_LOG(LogNFLEventBus, Warning,
				TEXT("BP %s denied: subsystem resolution failed."),
				ApiName);
			return false;
		}

		if (!ValidateBindingInputs(ApiName, TEXT("ListenerObj"), ChannelTag, ListenerObj))
		{
			return false;
		}

		Nfrrlib::EventBus::FListenerBinding Binding;
		Binding.FunctionName = FunctionName;

		const bool bResult = Subsystem->GetEventBus().AddListener(ChannelTag, ListenerObj, Binding);
		if (bResult)
		{
			RecordListenerHistory(Subsystem, ChannelTag, ListenerObj, FunctionName);
		}

		UE_LOG(LogNFLEventBus, Log,
			TEXT("BP %s result. Channel=%s Listener=%s Function=%s Success=%s"),
			ApiName,
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerObj),
			*FunctionName.ToString(),
			bResult ? TEXT("true") : TEXT("false"));
		return bResult;
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
 * @brief Blueprint facade wrapper for publisher registration with runtime history tracking.
 */
bool UEventBusBlueprintLibrary::AddPublisherValidated(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* PublisherObj,
	const FName DelegatePropertyName)
{
	return AddPublisherInternal(
		WorldContextObject,
		ChannelTag,
		PublisherObj,
		DelegatePropertyName,
		TEXT("AddPublisherValidated"));
}

/**
 * @brief Blueprint facade wrapper for publisher registration.
 */
bool UEventBusBlueprintLibrary::AddPublisher(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* PublisherObj,
	const FName DelegatePropertyName)
{
	return AddPublisherInternal(
		WorldContextObject,
		ChannelTag,
		PublisherObj,
		DelegatePropertyName,
		TEXT("AddPublisher"));
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
	if (!ValidateBindingInputs(TEXT("RemovePublisher"), TEXT("PublisherObj"), ChannelTag, PublisherObj))
	{
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
 * @brief Blueprint facade wrapper for listener registration with runtime history tracking.
 */
bool UEventBusBlueprintLibrary::AddListenerValidated(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* ListenerObj,
	const FName FunctionName)
{
	return AddListenerInternal(
		WorldContextObject,
		ChannelTag,
		ListenerObj,
		FunctionName,
		TEXT("AddListenerValidated"));
}

/**
 * @brief Blueprint facade wrapper for listener registration.
 */
bool UEventBusBlueprintLibrary::AddListener(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	UObject* ListenerObj,
	const FName FunctionName)
{
	return AddListenerInternal(
		WorldContextObject,
		ChannelTag,
		ListenerObj,
		FunctionName,
		TEXT("AddListener"));
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
	if (!ValidateBindingInputs(TEXT("RemoveListener"), TEXT("ListenerObj"), ChannelTag, ListenerObj))
	{
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
 * @brief Returns sorted, deduplicated listener functions recorded in runtime history.
 */
TArray<FName> UEventBusBlueprintLibrary::GetKnownListenerFunctions(
	UObject* WorldContextObject,
	const FGameplayTag ChannelTag,
	const TSubclassOf<UObject> ListenerClass)
{
	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP GetKnownListenerFunctions request. Channel=%s ListenerClass=%s"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass.Get()));

	UEventBusSubsystem* const Subsystem = ResolveSubsystem(WorldContextObject);
	if (!::IsValid(Subsystem))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP GetKnownListenerFunctions denied: subsystem resolution failed."));
		return {};
	}

	const UEventBusRegistryAsset* Registry = Subsystem->GetRuntimeRegistry();
	TArray<FName> KnownFunctions;
	if (::IsValid(Registry))
	{
		KnownFunctions = Registry->GetKnownListenerFunctions(ChannelTag, ListenerClass.Get());
	}
	else
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("BP GetKnownListenerFunctions warning: runtime registry is null, returning empty list."));
	}

	UE_LOG(LogNFLEventBus, Log,
		TEXT("BP GetKnownListenerFunctions result. Channel=%s ListenerClass=%s Count=%d"),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass.Get()),
		KnownFunctions.Num());
	return KnownFunctions;
}

/**
 * @brief Centralized subsystem resolve wrapper for all blueprint entry points.
 */
UEventBusSubsystem* UEventBusBlueprintLibrary::ResolveSubsystem(UObject* WorldContextObject)
{
	return ResolveEventBusSubsystem(WorldContextObject);
}
