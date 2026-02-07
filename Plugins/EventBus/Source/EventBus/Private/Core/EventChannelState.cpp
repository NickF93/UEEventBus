#include "Core/EventChannelState.h"

#include "UObject/UnrealType.h"

#include "EventBus/Core/EventBus.h"
#include "EventBus/Core/EventBusValidation.h"

namespace Nfrrlib::EventBus::Private
{
	/**
	 * @brief Constructs per-channel mutable runtime state.
	 */
	FEventChannelState::FEventChannelState(const bool bInOwnsPublisherDelegates)
		: bOwnsPublisherDelegates(bInOwnsPublisherDelegates)
	{
	}

	/**
	 * @brief Returns true when requested ownership policy matches channel policy.
	 */
	bool FEventChannelState::MatchesOwnershipPolicy(const bool bInOwnsPublisherDelegates) const
	{
		return bOwnsPublisherDelegates == bInOwnsPublisherDelegates;
	}

	bool FEventChannelState::OwnsPublisherDelegates() const
	{
		return bOwnsPublisherDelegates;
	}

	/**
	 * @brief Registers or updates one publisher and binds all compatible listeners to it.
	 */
	bool FEventChannelState::AddPublisher(UObject* PublisherObj, const FPublisherBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::ValidateObject(PublisherObj, Error) ||
			!FEventBusValidation::ValidateName(Binding.DelegatePropertyName, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisher failed. Error=%s Publisher=%s Delegate=%s"),
				LexToString(Error),
				*GetNameSafe(PublisherObj),
				*Binding.DelegatePropertyName.ToString());
			return false;
		}

		const FMulticastDelegateProperty* DelegateProperty =
			FEventBusValidation::ResolveDelegateProperty(PublisherObj, Binding.DelegatePropertyName, Error);
		if (DelegateProperty == nullptr)
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisher failed. Error=%s Publisher=%s Delegate=%s"),
				LexToString(Error),
				*GetNameSafe(PublisherObj),
				*Binding.DelegatePropertyName.ToString());
			return false;
		}

		const UFunction* DelegateSignature = DelegateProperty->SignatureFunction;
		if (DelegateSignature == nullptr)
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisher failed. Error=%s Publisher=%s Delegate=%s"),
				LexToString(EEventBusError::DelegatePropertyNotFound),
				*GetNameSafe(PublisherObj),
				*Binding.DelegatePropertyName.ToString());
			return false;
		}

		CleanupPublishers();
		CleanupListeners();
		RefreshChannelSignature();

		if (ChannelDelegateSignature != nullptr)
		{
			const bool bChannelCompatible =
				ChannelDelegateSignature->IsSignatureCompatibleWith(DelegateSignature) &&
				DelegateSignature->IsSignatureCompatibleWith(ChannelDelegateSignature);
			if (!bChannelCompatible)
			{
				UE_LOG(LogNFLEventBus, Warning,
					TEXT("AddPublisher failed. Error=%s Publisher=%s Delegate=%s ExistingDelegate=%s"),
					LexToString(EEventBusError::SignatureMismatch),
					*GetNameSafe(PublisherObj),
					*Binding.DelegatePropertyName.ToString(),
					*ChannelDelegatePropertyName.ToString());
				return false;
			}
		}

		for (const TPair<FListenerKey, FListenerEntry>& Pair : Listeners)
		{
			const FListenerEntry& ListenerEntry = Pair.Value;
			if (!::IsValid(ListenerEntry.Listener.Get()) || ListenerEntry.ListenerFunction == nullptr)
			{
				continue;
			}

			if (!FEventBusValidation::IsFunctionCompatibleWithDelegate(ListenerEntry.ListenerFunction, DelegateProperty, Error))
			{
				UE_LOG(LogNFLEventBus, Warning,
					TEXT("AddPublisher failed. Error=%s Publisher=%s Delegate=%s Listener=%s Function=%s"),
					LexToString(Error),
					*GetNameSafe(PublisherObj),
					*Binding.DelegatePropertyName.ToString(),
					*GetNameSafe(ListenerEntry.Listener.Get()),
					*ListenerEntry.FunctionName.ToString());
				return false;
			}
		}

		int32 ExistingIndex = Publishers.IndexOfByPredicate([PublisherObj](const FPublisherEntry& Entry)
		{
			return Entry.Publisher.Get() == PublisherObj;
		});

		if (ExistingIndex != INDEX_NONE)
		{
			UnbindAllListenersFromPublisher(Publishers[ExistingIndex]);
			Publishers[ExistingIndex].Publisher = PublisherObj;
			Publishers[ExistingIndex].DelegatePropertyName = Binding.DelegatePropertyName;
			Publishers[ExistingIndex].DelegateProperty = DelegateProperty;
		}
		else
		{
			FPublisherEntry NewEntry;
			NewEntry.Publisher = PublisherObj;
			NewEntry.DelegatePropertyName = Binding.DelegatePropertyName;
			NewEntry.DelegateProperty = DelegateProperty;
			Publishers.Add(MoveTemp(NewEntry));
			ExistingIndex = Publishers.Num() - 1;
		}

		if (ChannelDelegateSignature == nullptr)
		{
			ChannelDelegateSignature = DelegateSignature;
			ChannelDelegatePropertyName = Binding.DelegatePropertyName;
		}

		FPublisherEntry& PublisherEntry = Publishers[ExistingIndex];
		for (const TPair<FListenerKey, FListenerEntry>& Pair : Listeners)
		{
			BindListenerToPublisher(Pair.Value, PublisherEntry);
		}

		return true;
	}

	/**
	 * @brief Removes all bindings for one publisher from this channel.
	 */
	bool FEventChannelState::RemovePublisher(UObject* PublisherObj)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::ValidateObject(PublisherObj, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemovePublisher failed. Error=%s Publisher=%s"),
				LexToString(Error),
				*GetNameSafe(PublisherObj));
			return false;
		}

		CleanupPublishers();
		CleanupListeners();

		bool bRemoved = false;
		for (int32 Index = Publishers.Num() - 1; Index >= 0; --Index)
		{
			if (Publishers[Index].Publisher.Get() == PublisherObj)
			{
				UnbindAllListenersFromPublisher(Publishers[Index]);
				Publishers.RemoveAtSwap(Index);
				bRemoved = true;
			}
		}

		RefreshChannelSignature();
		return bRemoved;
	}

	/**
	 * @brief Registers or updates one listener callback and binds it to all publishers.
	 */
	bool FEventChannelState::AddListener(UObject* ListenerObj, const FListenerBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		const UFunction* ListenerFunction = nullptr;
		FScriptDelegate Callback;
		if (!FEventBusValidation::BuildListenerBinding(ListenerObj, Binding.FunctionName, ListenerFunction, Callback, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListener failed. Error=%s Listener=%s Func=%s"),
				LexToString(Error),
				*GetNameSafe(ListenerObj),
				*Binding.FunctionName.ToString());
			return false;
		}

		CleanupPublishers();
		CleanupListeners();
		RefreshChannelSignature();

		if (ChannelDelegateSignature != nullptr)
		{
			const bool bCompatible =
				ListenerFunction->IsSignatureCompatibleWith(ChannelDelegateSignature) &&
				ChannelDelegateSignature->IsSignatureCompatibleWith(ListenerFunction);
			if (!bCompatible)
			{
				UE_LOG(LogNFLEventBus, Warning,
					TEXT("AddListener failed. Error=%s Listener=%s Func=%s Delegate=%s"),
					LexToString(EEventBusError::SignatureMismatch),
					*GetNameSafe(ListenerObj),
					*Binding.FunctionName.ToString(),
					*ChannelDelegatePropertyName.ToString());
				return false;
			}
		}

		FListenerKey ListenerKey;
		ListenerKey.ListenerObjectKey = FObjectKey(ListenerObj);
		ListenerKey.FunctionName = Binding.FunctionName;

		if (FListenerEntry* Existing = Listeners.Find(ListenerKey))
		{
			for (FPublisherEntry& PublisherEntry : Publishers)
			{
				UnbindListenerFromPublisher(*Existing, PublisherEntry);
			}
		}

		FListenerEntry NewEntry;
		NewEntry.ListenerKey = ListenerKey;
		NewEntry.Listener = ListenerObj;
		NewEntry.FunctionName = Binding.FunctionName;
		NewEntry.ListenerFunction = ListenerFunction;
		NewEntry.Callback = Callback;
		Listeners.Add(ListenerKey, MoveTemp(NewEntry));

		FListenerEntry* AddedEntry = Listeners.Find(ListenerKey);
		check(AddedEntry != nullptr);

		for (FPublisherEntry& PublisherEntry : Publishers)
		{
			BindListenerToPublisher(*AddedEntry, PublisherEntry);
		}

		return true;
	}

	/**
	 * @brief Removes one listener callback or object-wide callbacks based on ownership mode.
	 */
	bool FEventChannelState::RemoveListener(UObject* ListenerObj, const FListenerBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::ValidateObject(ListenerObj, Error) ||
			!FEventBusValidation::ValidateName(Binding.FunctionName, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemoveListener failed. Error=%s Listener=%s Func=%s"),
				LexToString(Error),
				*GetNameSafe(ListenerObj),
				*Binding.FunctionName.ToString());
			return false;
		}

		CleanupPublishers();
		CleanupListeners();

		FListenerKey TargetKey;
		TargetKey.ListenerObjectKey = FObjectKey(ListenerObj);
		TargetKey.FunctionName = Binding.FunctionName;

		TArray<FListenerKey> ListenerKeysToRemove;
		if (bOwnsPublisherDelegates)
		{
			for (const TPair<FListenerKey, FListenerEntry>& Pair : Listeners)
			{
				if (Pair.Key.ListenerObjectKey == TargetKey.ListenerObjectKey)
				{
					ListenerKeysToRemove.Add(Pair.Key);
				}
			}
		}
		else
		{
			if (Listeners.Contains(TargetKey))
			{
				ListenerKeysToRemove.Add(TargetKey);
			}
		}

		if (ListenerKeysToRemove.IsEmpty())
		{
			return false;
		}

		for (const FListenerKey& ListenerKey : ListenerKeysToRemove)
		{
			FListenerEntry* Entry = Listeners.Find(ListenerKey);
			if (!Entry)
			{
				continue;
			}

			for (FPublisherEntry& PublisherEntry : Publishers)
			{
				UnbindListenerFromPublisher(*Entry, PublisherEntry);
			}
		}

		for (const FListenerKey& ListenerKey : ListenerKeysToRemove)
		{
			Listeners.Remove(ListenerKey);
		}

		return true;
	}

	/**
	 * @brief Fully unbinds all listener callbacks from all publishers and clears channel state.
	 */
	void FEventChannelState::ClearAndUnbind()
	{
		CleanupPublishers();
		CleanupListeners();

		for (FPublisherEntry& PublisherEntry : Publishers)
		{
			UnbindAllListenersFromPublisher(PublisherEntry);
		}

		Publishers.Reset();
		Listeners.Reset();
		ChannelDelegateSignature = nullptr;
		ChannelDelegatePropertyName = NAME_None;
	}

	/**
	 * @brief Removes stale publishers that are no longer valid and refreshes channel signature cache.
	 */
	void FEventChannelState::CleanupPublishers()
	{
		for (int32 Index = Publishers.Num() - 1; Index >= 0; --Index)
		{
			if (!::IsValid(Publishers[Index].Publisher.Get()))
			{
				Publishers.RemoveAtSwap(Index);
			}
		}

		RefreshChannelSignature();
	}

	/**
	 * @brief Removes stale listeners and detaches stale callbacks from live publishers.
	 */
	void FEventChannelState::CleanupListeners()
	{
		for (auto It = Listeners.CreateIterator(); It; ++It)
		{
			if (!::IsValid(It.Value().Listener.Get()))
			{
				const FScriptDelegate StaleCallback = It.Value().Callback;
				if (StaleCallback.IsBound())
				{
					for (FPublisherEntry& PublisherEntry : Publishers)
					{
						UObject* PublisherObj = PublisherEntry.Publisher.Get();
						if (::IsValid(PublisherObj) && PublisherEntry.DelegateProperty != nullptr)
						{
							PublisherEntry.DelegateProperty->RemoveDelegate(StaleCallback, PublisherObj);
						}
					}
				}

				It.RemoveCurrent();
			}
		}
	}

	/**
	 * @brief Recomputes channel delegate signature from currently valid publishers.
	 */
	void FEventChannelState::RefreshChannelSignature()
	{
		ChannelDelegateSignature = nullptr;
		ChannelDelegatePropertyName = NAME_None;

		for (const FPublisherEntry& PublisherEntry : Publishers)
		{
			if (!::IsValid(PublisherEntry.Publisher.Get()) || PublisherEntry.DelegateProperty == nullptr)
			{
				continue;
			}

			const UFunction* PublisherSignature = PublisherEntry.DelegateProperty->SignatureFunction;
			if (PublisherSignature == nullptr)
			{
				continue;
			}

			if (ChannelDelegateSignature == nullptr)
			{
				ChannelDelegateSignature = PublisherSignature;
				ChannelDelegatePropertyName = PublisherEntry.DelegatePropertyName;
				continue;
			}

			const bool bCompatible =
				ChannelDelegateSignature->IsSignatureCompatibleWith(PublisherSignature) &&
				PublisherSignature->IsSignatureCompatibleWith(ChannelDelegateSignature);
			if (!bCompatible)
			{
				UE_LOG(LogNFLEventBus, Warning,
					TEXT("Channel signature drift detected. ExistingDelegate=%s NewDelegate=%s"),
					*ChannelDelegatePropertyName.ToString(),
					*PublisherEntry.DelegatePropertyName.ToString());
			}
		}
	}

	/**
	 * @brief Removes callback bindings from one publisher according to channel ownership policy.
	 */
	void FEventChannelState::RemoveBinding(
		UObject* PublisherObj,
		const FMulticastDelegateProperty* DelegateProperty,
		UObject* ListenerObj,
		const FScriptDelegate& Callback) const
	{
		if (!::IsValid(PublisherObj) || DelegateProperty == nullptr)
		{
			return;
		}

		if (bOwnsPublisherDelegates)
		{
			for (const TPair<FListenerKey, FListenerEntry>& Pair : Listeners)
			{
				const FListenerEntry& ListenerEntry = Pair.Value;
				if (ListenerEntry.Listener.Get() == ListenerObj && ListenerEntry.Callback.IsBound())
				{
					DelegateProperty->RemoveDelegate(ListenerEntry.Callback, PublisherObj);
				}
			}
			return;
		}

		if (Callback.IsBound())
		{
			DelegateProperty->RemoveDelegate(Callback, PublisherObj);
		}
	}

	/**
	 * @brief Binds listener callback to one publisher after duplicate-safe pre-unbind.
	 */
	void FEventChannelState::BindListenerToPublisher(const FListenerEntry& ListenerEntry, FPublisherEntry& PublisherEntry) const
	{
		UObject* PublisherObj = PublisherEntry.Publisher.Get();
		UObject* ListenerObj = ListenerEntry.Listener.Get();
		if (!::IsValid(PublisherObj) || !::IsValid(ListenerObj) || PublisherEntry.DelegateProperty == nullptr)
		{
			return;
		}

		RemoveBinding(PublisherObj, PublisherEntry.DelegateProperty, ListenerObj, ListenerEntry.Callback);
		PublisherEntry.DelegateProperty->AddDelegate(ListenerEntry.Callback, PublisherObj);
	}

	/**
	 * @brief Unbinds listener callback from one publisher.
	 */
	void FEventChannelState::UnbindListenerFromPublisher(const FListenerEntry& ListenerEntry, FPublisherEntry& PublisherEntry) const
	{
		UObject* PublisherObj = PublisherEntry.Publisher.Get();
		UObject* ListenerObj = ListenerEntry.Listener.Get();
		if (!::IsValid(PublisherObj) || !::IsValid(ListenerObj) || PublisherEntry.DelegateProperty == nullptr)
		{
			return;
		}

		RemoveBinding(PublisherObj, PublisherEntry.DelegateProperty, ListenerObj, ListenerEntry.Callback);
	}

	/**
	 * @brief Unbinds all tracked listener callbacks from one publisher.
	 */
	void FEventChannelState::UnbindAllListenersFromPublisher(FPublisherEntry& PublisherEntry) const
	{
		for (const TPair<FListenerKey, FListenerEntry>& Pair : Listeners)
		{
			UnbindListenerFromPublisher(Pair.Value, PublisherEntry);
		}
	}
} // namespace Nfrrlib::EventBus::Private
