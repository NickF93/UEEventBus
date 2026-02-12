#include "EventBus/BP/EventBusRegistryAsset.h"

#include "Containers/Set.h"

#include "EventBus/Core/EventBus.h"

namespace
{
	constexpr int32 MaxPublisherHistoryEntries = 512;
	constexpr int32 MaxListenerHistoryEntries = 512;

	void SortAndUniqueNames(TArray<FName>& Names)
	{
		TSet<FName> UniqueNames;
		for (const FName Name : Names)
		{
			if (!Name.IsNone())
			{
				UniqueNames.Add(Name);
			}
		}

		Names = UniqueNames.Array();
		Names.Sort([](const FName& A, const FName& B)
		{
			return A.Compare(B) < 0;
		});
	}

	template <typename TEntryType>
	void TrimOldestEntries(TArray<TEntryType>& Entries, const int32 MaxEntries)
	{
		if (MaxEntries <= 0 || Entries.Num() <= MaxEntries)
		{
			return;
		}

		const int32 OverflowCount = Entries.Num() - MaxEntries;
		Entries.RemoveAt(0, OverflowCount, EAllowShrinking::No);
	}
}

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

	PublisherHistory.RemoveAll([](const FEventBusPublisherHistoryEntry& Entry)
	{
		return !Entry.ChannelTag.IsValid() ||
			!::IsValid(Entry.PublisherClass.Get()) ||
			Entry.DelegatePropertyName.IsNone();
	});

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
	TrimOldestEntries(PublisherHistory, MaxPublisherHistoryEntries);

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

	ListenerHistory.RemoveAll([](const FEventBusListenerHistoryEntry& Entry)
	{
		return !Entry.ChannelTag.IsValid() ||
			!::IsValid(Entry.ListenerClass.Get());
	});

	for (FEventBusListenerHistoryEntry& Entry : ListenerHistory)
	{
		SortAndUniqueNames(Entry.KnownFunctions);
	}

	ListenerHistory.RemoveAll([](const FEventBusListenerHistoryEntry& Entry)
	{
		return Entry.KnownFunctions.IsEmpty();
	});

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

	FoundEntry->KnownFunctions.Add(FunctionName);
	SortAndUniqueNames(FoundEntry->KnownFunctions);
	const int32 KnownFunctionsCount = FoundEntry->KnownFunctions.Num();

	TrimOldestEntries(ListenerHistory, MaxListenerHistoryEntries);

	UE_LOG(LogNFLEventBus, Log,
		TEXT("Registry RecordListenerBinding updated. Registry=%s Channel=%s ListenerClass=%s Function=%s KnownCount=%d"),
		*GetNameSafe(this),
		*ChannelTag.ToString(),
		*GetNameSafe(ListenerClass),
		*FunctionName.ToString(),
		KnownFunctionsCount);
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
			for (const FName FunctionName : Entry.KnownFunctions)
			{
				if (!FunctionName.IsNone())
				{
					Result.Add(FunctionName);
				}
			}
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

/**
 * @brief Clears all runtime history containers.
 */
void UEventBusRegistryAsset::ResetHistory()
{
	const int32 PublisherCount = PublisherHistory.Num();
	const int32 ListenerCount = ListenerHistory.Num();

	PublisherHistory.Reset();
	ListenerHistory.Reset();

	UE_LOG(LogNFLEventBus, Log,
		TEXT("Registry ResetHistory completed. Registry=%s RemovedPublishers=%d RemovedListeners=%d"),
		*GetNameSafe(this),
		PublisherCount,
		ListenerCount);
}
