#include "EventBus/BP/EventBusRegistryAsset.h"

#include "Containers/Set.h"

#include "EventBus/Core/EventBus.h"

/**
 * @brief Records one publisher binding in history if valid and not already present.
 */
void UEventBusRegistryAsset::RecordPublisherBinding(
	const FGameplayTag& ChannelTag,
	UClass* PublisherClass,
	const FName DelegatePropertyName)
{
	if (!ChannelTag.IsValid() || !::IsValid(PublisherClass) || DelegatePropertyName.IsNone())
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry RecordPublisherBinding invalid input. Registry=%s Channel=%s PublisherClass=%s Delegate=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(PublisherClass),
			*DelegatePropertyName.ToString());
		return;
	}

	for (const FEventBusPublisherHistoryEntry& Entry : PublisherHistory)
	{
		if (Entry.ChannelTag == ChannelTag &&
			Entry.PublisherClass.Get() == PublisherClass &&
			Entry.DelegatePropertyName == DelegatePropertyName)
		{
			return;
		}
	}

	FEventBusPublisherHistoryEntry NewEntry;
	NewEntry.ChannelTag = ChannelTag;
	NewEntry.PublisherClass = PublisherClass;
	NewEntry.DelegatePropertyName = DelegatePropertyName;
	PublisherHistory.Add(MoveTemp(NewEntry));

	UE_LOG(LogNFLEventBus, Log,
		TEXT("Registry RecordPublisherBinding added. Registry=%s Channel=%s PublisherClass=%s Delegate=%s Total=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(PublisherClass),
		*DelegatePropertyName.ToString(),
		PublisherHistory.Num());
}

/**
 * @brief Records one listener function binding in history.
 */
void UEventBusRegistryAsset::RecordListenerBinding(
	const FGameplayTag& ChannelTag,
	UClass* ListenerClass,
	const FName FunctionName)
{
	if (!ChannelTag.IsValid() || !::IsValid(ListenerClass) || FunctionName.IsNone())
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry RecordListenerBinding invalid input. Registry=%s Channel=%s ListenerClass=%s Function=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerClass),
			*FunctionName.ToString());
		return;
	}

	FEventBusListenerHistoryEntry* FoundEntry = ListenerHistory.FindByPredicate(
		[&ChannelTag, ListenerClass](const FEventBusListenerHistoryEntry& Entry)
		{
			return Entry.ChannelTag == ChannelTag && Entry.ListenerClass.Get() == ListenerClass;
		});

	if (!FoundEntry)
	{
		FEventBusListenerHistoryEntry NewEntry;
		NewEntry.ChannelTag = ChannelTag;
		NewEntry.ListenerClass = ListenerClass;
		ListenerHistory.Add(MoveTemp(NewEntry));
		FoundEntry = &ListenerHistory.Last();
	}

	if (!FoundEntry->KnownFunctions.Contains(FunctionName))
	{
		FoundEntry->KnownFunctions.Add(FunctionName);
		FoundEntry->KnownFunctions.Sort([](const FName& A, const FName& B)
		{
			return A.Compare(B) < 0;
		});
	}

	UE_LOG(LogNFLEventBus, Log,
		TEXT("Registry RecordListenerBinding updated. Registry=%s Channel=%s ListenerClass=%s Function=%s KnownCount=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass),
		*FunctionName.ToString(),
		FoundEntry->KnownFunctions.Num());
}

/**
 * @brief Returns deduplicated/sorted listener functions recorded for class/channel.
 */
TArray<FName> UEventBusRegistryAsset::GetKnownListenerFunctions(const FGameplayTag& ChannelTag, UClass* ListenerClass) const
{
	TArray<FName> Result;
	if (!ChannelTag.IsValid() || !::IsValid(ListenerClass))
	{
		UE_LOG(LogNFLEventBus, Warning,
			TEXT("Registry GetKnownListenerFunctions invalid input. Registry=%s Channel=%s ListenerClass=%s"),
			*GetNameSafe(this),
			*ChannelTag.ToString(),
			*GetNameSafe(ListenerClass));
		return Result;
	}

	for (const FEventBusListenerHistoryEntry& Entry : ListenerHistory)
	{
		UClass* const EntryClass = Entry.ListenerClass.Get();
		if (!::IsValid(EntryClass))
		{
			continue;
		}

		// Keep lookup strict to class-local history so picker results do not include inherited members.
		if (Entry.ChannelTag == ChannelTag && ListenerClass == EntryClass)
		{
			Result.Append(Entry.KnownFunctions);
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
		TEXT("Registry GetKnownListenerFunctions result. Registry=%s Channel=%s ListenerClass=%s Count=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass),
		Result.Num());
	return Result;
}
