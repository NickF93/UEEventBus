#include "EventBus/BP/EventBusRegistryAsset.h"

#include "Containers/Set.h"

#include "EventBus/Core/EventBus.h"

/**
 * @brief Checks publisher allowlist rule match for channel/class/delegate property tuple.
 */
bool UEventBusRegistryAsset::IsPublisherAllowed(
	const FGameplayTag& ChannelTag,
	UClass* PublisherClass,
	const FName DelegatePropertyName) const
{
	if (!ChannelTag.IsValid() || !::IsValid(PublisherClass) || DelegatePropertyName.IsNone())
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry IsPublisherAllowed invalid input. Registry=%s Channel=%s PublisherClass=%s Delegate=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherClass),
			*DelegatePropertyName.ToString());
		return false;
	}

	int32 ScannedRules = 0;
	for (const FEventBusPublisherRule& Rule : PublisherRules)
	{
		++ScannedRules;
		if (!Rule.ChannelTag.IsValid() || Rule.DelegatePropertyName.IsNone())
		{
			continue;
		}

		UClass* const RuleClass = Rule.PublisherClass.Get();
		if (!::IsValid(RuleClass))
		{
			continue;
		}

		if (Rule.ChannelTag == ChannelTag &&
			PublisherClass->IsChildOf(RuleClass) &&
			Rule.DelegatePropertyName == DelegatePropertyName)
		{
			UE_LOG(LogNFLEventBus, Log,
				TEXT("Registry IsPublisherAllowed matched. Registry=%s Channel=%s PublisherClass=%s Delegate=%s RuleClass=%s"),
				*GetNameSafe(this),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherClass),
				*DelegatePropertyName.ToString(),
				*GetNameSafe(RuleClass));
			return true;
		}
	}

	UE_LOG(LogNFLEventBus, Warning,
		TEXT("Registry IsPublisherAllowed denied. Registry=%s Channel=%s PublisherClass=%s Delegate=%s RulesScanned=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherClass),
		*DelegatePropertyName.ToString(),
		ScannedRules);
	return false;
}

/**
 * @brief Checks listener allowlist rule match for channel/class/function tuple.
 */
bool UEventBusRegistryAsset::IsListenerAllowed(
	const FGameplayTag& ChannelTag,
	UClass* ListenerClass,
	const FName FunctionName) const
{
	if (!ChannelTag.IsValid() || !::IsValid(ListenerClass) || FunctionName.IsNone())
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry IsListenerAllowed invalid input. Registry=%s Channel=%s ListenerClass=%s Function=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerClass),
			*FunctionName.ToString());
		return false;
	}

	int32 ScannedRules = 0;
	for (const FEventBusListenerRule& Rule : ListenerRules)
	{
		++ScannedRules;
		if (!Rule.ChannelTag.IsValid())
		{
			continue;
		}

		UClass* const RuleClass = Rule.ListenerClass.Get();
		if (!::IsValid(RuleClass))
		{
			continue;
		}

		if (Rule.ChannelTag != ChannelTag || !ListenerClass->IsChildOf(RuleClass))
		{
			continue;
		}

		if (Rule.AllowedFunctions.Contains(FunctionName))
		{
			UE_LOG(LogNFLEventBus, Log,
				TEXT("Registry IsListenerAllowed matched. Registry=%s Channel=%s ListenerClass=%s Function=%s RuleClass=%s"),
				*GetNameSafe(this),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerClass),
				*FunctionName.ToString(),
				*GetNameSafe(RuleClass));
			return true;
		}
	}

	UE_LOG(LogNFLEventBus, Warning,
		TEXT("Registry IsListenerAllowed denied. Registry=%s Channel=%s ListenerClass=%s Function=%s RulesScanned=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass),
		*FunctionName.ToString(),
		ScannedRules);
	return false;
}

/**
 * @brief Collects, deduplicates, and sorts allowlisted listener function names.
 */
TArray<FName> UEventBusRegistryAsset::GetAllowedListenerFunctions(const FGameplayTag& ChannelTag, UClass* ListenerClass) const
{
	TArray<FName> Result;
	if (!ChannelTag.IsValid() || !::IsValid(ListenerClass))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry GetAllowedListenerFunctions invalid input. Registry=%s Channel=%s ListenerClass=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerClass));
		return Result;
	}

	for (const FEventBusListenerRule& Rule : ListenerRules)
	{
		UClass* const RuleClass = Rule.ListenerClass.Get();
		if (!::IsValid(RuleClass))
		{
			continue;
		}

		if (Rule.ChannelTag == ChannelTag && ListenerClass->IsChildOf(RuleClass))
		{
			Result.Append(Rule.AllowedFunctions);
		}
	}

	TSet<FName> UniqueNames;
	for (const FName Name : Result)
	{
		UniqueNames.Add(Name);
	}

	Result = UniqueNames.Array();
	Result.Sort([](const FName& A, const FName& B)
	{
		return A.Compare(B) < 0;
	});

	UE_LOG(LogNFLEventBus, Log,
		TEXT("Registry GetAllowedListenerFunctions result. Registry=%s Channel=%s ListenerClass=%s Count=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass),
		Result.Num());
	return Result;
}
