#include "EventBus/BP/EventBusRegistryAsset.h"

#include "Containers/Set.h"

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
		return false;
	}

	for (const FEventBusPublisherRule& Rule : PublisherRules)
	{
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
			return true;
		}
	}

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
		return false;
	}

	for (const FEventBusListenerRule& Rule : ListenerRules)
	{
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
			return true;
		}
	}

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
	return Result;
}
