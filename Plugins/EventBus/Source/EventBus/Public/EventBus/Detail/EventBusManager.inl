#pragma once

namespace Nfrrlib
{
	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE void FEventBusManager::AddPublisher(typename TEventTag::PublisherType* PublisherObj)
	{
		if (!EnsureGameThread(TEXT("AddPublisher")))
		{
			return;
		}

		if (!EnsureChannelRoute<TEventTag, bOwnsPublisherDelegates>())
		{
			return;
		}

		auto& Bus = GetOrCreateBus<TEventTag, bOwnsPublisherDelegates>();
		Bus.RegisterPublisher(TEventTag::GetPublisherId(), PublisherObj, TEventTag::DelegateMember);
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE void FEventBusManager::RemovePublisher(UObject* PublisherObj)
	{
		if (!EnsureGameThread(TEXT("RemovePublisher")))
		{
			return;
		}

		TEventBus<TEventTag, bOwnsPublisherDelegates>* Bus = GetBusIfExists<TEventTag, bOwnsPublisherDelegates>();
		if (Nfrrlib::IsValid(Bus))
		{
			Bus->UnregisterPublisher(PublisherObj);
		}
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates, CUObjectDerived TListener>
	FORCEINLINE void FEventBusManager::AddListener(TListener* ListenerObj, FName FuncName)
	{
		if (!EnsureGameThread(TEXT("AddListener")))
		{
			return;
		}

		auto& Bus = GetOrCreateBus<TEventTag, bOwnsPublisherDelegates>();

		const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
		const FScriptDelegate Callback = Detail::MakeScriptDelegate(ListenerObj, FuncName);
		Bus.RegisterListener(ListenerId, ListenerObj, Callback);
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates, CUObjectDerived TListener>
	FORCEINLINE void FEventBusManager::RemoveListener(TListener* ListenerObj, FName FuncName)
	{
		if (!EnsureGameThread(TEXT("RemoveListener")))
		{
			return;
		}

		TEventBus<TEventTag, bOwnsPublisherDelegates>* Bus = GetBusIfExists<TEventTag, bOwnsPublisherDelegates>();
		if (Nfrrlib::IsValid(Bus))
		{
			const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
			Bus->UnregisterListener(ListenerId, ListenerObj);
		}
	}

	template <CUObjectDerived TListener>
	FORCEINLINE bool FEventBusManager::AddListenerByChannel(const FGameplayTag& ChannelTag, TListener* ListenerObj, FName FuncName)
	{
		return AddListenerByChannelImpl(ChannelTag, ListenerObj, FuncName);
	}

	template <CUObjectDerived TListener>
	FORCEINLINE bool FEventBusManager::RemoveListenerByChannel(const FGameplayTag& ChannelTag, TListener* ListenerObj, FName FuncName)
	{
		return RemoveListenerByChannelImpl(ChannelTag, ListenerObj, FuncName);
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::RegisterChannel()
	{
		if (!EnsureGameThread(TEXT("RegisterChannel")))
		{
			return false;
		}

		return EnsureChannelRoute<TEventTag, bOwnsPublisherDelegates>();
	}

	template <typename TEventTag>
	FORCEINLINE const void* FEventBusManager::GetTagTypeToken()
	{
		static constexpr uint8 Token = 0;
		return &Token;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::AddPublisherByChannelOp(FEventBusManager& Manager, UObject* PublisherObj)
	{
		using FPublisherType = typename TEventTag::PublisherType;

		FPublisherType* TypedPublisher = Cast<FPublisherType>(PublisherObj);
		if (!Nfrrlib::IsValid(TypedPublisher))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByChannelOp failed: invalid publisher type. Publisher=%s"),
				*GetNameSafe(PublisherObj));
			return false;
		}

		auto& Bus = Manager.GetOrCreateBus<TEventTag, bOwnsPublisherDelegates>();
		return Bus.RegisterPublisher(TEventTag::GetPublisherId(), TypedPublisher, TEventTag::DelegateMember);
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::RemovePublisherByChannelOp(FEventBusManager& Manager, UObject* PublisherObj)
	{
		TEventBus<TEventTag, bOwnsPublisherDelegates>* Bus = Manager.GetBusIfExists<TEventTag, bOwnsPublisherDelegates>();
		if (Nfrrlib::IsValid(Bus))
		{
			return Bus->UnregisterPublisher(PublisherObj);
		}

		return false;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::AddListenerByChannelOp(FEventBusManager& Manager, UObject* ListenerObj, FName FuncName)
	{
		if (!Nfrrlib::IsValid(ListenerObj) || FuncName.IsNone())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannelOp failed: Listener=%s Func=%s"),
				*GetNameSafe(ListenerObj),
				*FuncName.ToString());
			return false;
		}

		auto& Bus = Manager.GetOrCreateBus<TEventTag, bOwnsPublisherDelegates>();
		const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
		const FScriptDelegate Callback = Detail::MakeScriptDelegate(ListenerObj, FuncName);
		return Bus.RegisterListener(ListenerId, ListenerObj, Callback);
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::RemoveListenerByChannelOp(FEventBusManager& Manager, UObject* ListenerObj, FName FuncName)
	{
		TEventBus<TEventTag, bOwnsPublisherDelegates>* Bus = Manager.GetBusIfExists<TEventTag, bOwnsPublisherDelegates>();
		if (Nfrrlib::IsValid(Bus))
		{
			const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
			return Bus->UnregisterListener(ListenerId, ListenerObj);
		}

		return false;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE bool FEventBusManager::EnsureChannelRoute()
	{
		const FGameplayTag ChannelTag = TEventTag::GetPublisherId();
		if (!ChannelTag.IsValid())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("EnsureChannelRoute failed: invalid channel for event tag type."));
			return false;
		}

		const void* ExpectedTypeToken = GetTagTypeToken<TEventTag>();
		FChannelRoute* ExistingRoute = ChannelRoutes.Find(ChannelTag);
		if (Nfrrlib::IsValid(ExistingRoute))
		{
			if (ExistingRoute->TagTypeToken != ExpectedTypeToken ||
				ExistingRoute->bOwnsPublisherDelegates != bOwnsPublisherDelegates)
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("EnsureChannelRoute conflict: Channel=%s ExistingType=%p NewType=%p ExistingOwns=%d NewOwns=%d"),
					*ChannelTag.ToString(),
					ExistingRoute->TagTypeToken,
					ExpectedTypeToken,
					ExistingRoute->bOwnsPublisherDelegates,
					bOwnsPublisherDelegates);
				return false;
			}

			return true;
		}

		FChannelRoute NewRoute;
		NewRoute.TagTypeToken = ExpectedTypeToken;
		NewRoute.bOwnsPublisherDelegates = bOwnsPublisherDelegates;
		NewRoute.AddPublisherOp = &AddPublisherByChannelOp<TEventTag, bOwnsPublisherDelegates>;
		NewRoute.RemovePublisherOp = &RemovePublisherByChannelOp<TEventTag, bOwnsPublisherDelegates>;
		NewRoute.AddListenerOp = &AddListenerByChannelOp<TEventTag, bOwnsPublisherDelegates>;
		NewRoute.RemoveListenerOp = &RemoveListenerByChannelOp<TEventTag, bOwnsPublisherDelegates>;
		ChannelRoutes.Add(ChannelTag, MoveTemp(NewRoute));
		return true;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE FEventBusManager::FBusKey FEventBusManager::MakeKey() const
	{
		FBusKey Key;
		Key.TagTypeToken = GetTagTypeToken<TEventTag>();
		Key.bOwnsPublisherDelegates = bOwnsPublisherDelegates;
		return Key;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE TEventBus<TEventTag, bOwnsPublisherDelegates>& FEventBusManager::GetOrCreateBus()
	{
		const FBusKey Key = MakeKey<TEventTag, bOwnsPublisherDelegates>();
		TUniquePtr<IBusHolder>* Found = Buses.Find(Key);
		if (Nfrrlib::IsValid(Found))
		{
			return static_cast<TBusHolder<TEventTag, bOwnsPublisherDelegates>*>(Found->Get())->Bus;
		}

		TUniquePtr<TBusHolder<TEventTag, bOwnsPublisherDelegates>> NewHolder =
			MakeUnique<TBusHolder<TEventTag, bOwnsPublisherDelegates>>();

		TBusHolder<TEventTag, bOwnsPublisherDelegates>* RawHolder = NewHolder.Get();
		Buses.Add(Key, MoveTemp(NewHolder));
		return RawHolder->Bus;
	}

	template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
	FORCEINLINE TEventBus<TEventTag, bOwnsPublisherDelegates>* FEventBusManager::GetBusIfExists()
	{
		const FBusKey Key = MakeKey<TEventTag, bOwnsPublisherDelegates>();
		TUniquePtr<IBusHolder>* Found = Buses.Find(Key);
		if (Nfrrlib::IsValid(Found))
		{
			return &static_cast<TBusHolder<TEventTag, bOwnsPublisherDelegates>*>(Found->Get())->Bus;
		}

		return nullptr;
	}
} // namespace Nfrrlib
