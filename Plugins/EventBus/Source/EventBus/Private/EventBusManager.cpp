#include "EventBus/EventBusManager.h"

#include "UObject/UnrealType.h"

namespace Nfrrlib
{
	class FEventBusManager::FRuntimeChannelBus final
	{
	public:
		struct FPublisherEntry final
		{
			TWeakObjectPtr<UObject> Publisher;
		};

		struct FListenerEntry final
		{
			TWeakObjectPtr<UObject> Listener;
			FScriptDelegate Callback;
		};

		bool RegisterPublisher(UObject* PublisherObj, const FMulticastDelegateProperty* DelegateProperty)
		{
			if (!Nfrrlib::IsValid(PublisherObj) || !Nfrrlib::IsValid(DelegateProperty))
			{
				return false;
			}

			CleanupListeners();

			int32 ExistingIndex = Publishers.IndexOfByPredicate([PublisherObj](const FPublisherEntry& Entry)
			{
				return Entry.Publisher.Get() == PublisherObj;
			});

			if (ExistingIndex != INDEX_NONE)
			{
				UnbindPublisherFromAllListeners(Publishers[ExistingIndex], DelegateProperty);
				Publishers[ExistingIndex].Publisher = PublisherObj;
			}
			else
			{
				FPublisherEntry NewEntry;
				NewEntry.Publisher = PublisherObj;
				Publishers.Add(MoveTemp(NewEntry));
				ExistingIndex = Publishers.Num() - 1;
			}

			UObject* Publisher = Publishers[ExistingIndex].Publisher.Get();
			if (!Nfrrlib::IsValid(Publisher))
			{
				return false;
			}

			for (const TPair<FGuid, FListenerEntry>& Pair : Listeners)
			{
				const FListenerEntry& ListenerEntry = Pair.Value;
				if (!Nfrrlib::IsValid(ListenerEntry.Listener) || !ListenerEntry.Callback.IsBound())
				{
					continue;
				}

				DelegateProperty->RemoveDelegate(ListenerEntry.Callback, Publisher);
				DelegateProperty->AddDelegate(ListenerEntry.Callback, Publisher);
			}

			return true;
		}

		bool UnregisterPublisher(UObject* PublisherObj, const FMulticastDelegateProperty* DelegateProperty)
		{
			if (!Nfrrlib::IsValid(PublisherObj) || !Nfrrlib::IsValid(DelegateProperty))
			{
				return false;
			}

			bool bRemovedAny = false;
			for (int32 Index = Publishers.Num() - 1; Index >= 0; --Index)
			{
				if (Publishers[Index].Publisher.Get() == PublisherObj)
				{
					UnbindPublisherFromAllListeners(Publishers[Index], DelegateProperty);
					Publishers.RemoveAtSwap(Index);
					bRemovedAny = true;
				}
			}

			return bRemovedAny;
		}

		bool RegisterListener(FGuid ListenerId, UObject* ListenerObj, const FScriptDelegate& Callback, const FMulticastDelegateProperty* DelegateProperty)
		{
			if (!Nfrrlib::IsValid(ListenerObj) || !ListenerId.IsValid() || !Callback.IsBound() || !Nfrrlib::IsValid(DelegateProperty))
			{
				return false;
			}

			FListenerEntry* OldEntry = Listeners.Find(ListenerId);
			if (Nfrrlib::IsValid(OldEntry))
			{
				for (const FPublisherEntry& PublisherEntry : Publishers)
				{
					UObject* Publisher = PublisherEntry.Publisher.Get();
					if (Nfrrlib::IsValid(Publisher) && OldEntry->Callback.IsBound())
					{
						DelegateProperty->RemoveDelegate(OldEntry->Callback, Publisher);
					}
				}
			}

			FListenerEntry NewEntry;
			NewEntry.Listener = ListenerObj;
			NewEntry.Callback = Callback;
			Listeners.Add(ListenerId, MoveTemp(NewEntry));

			CleanupPublishers();
			for (const FPublisherEntry& PublisherEntry : Publishers)
			{
				UObject* Publisher = PublisherEntry.Publisher.Get();
				if (!Nfrrlib::IsValid(Publisher))
				{
					continue;
				}

				DelegateProperty->RemoveDelegate(Callback, Publisher);
				DelegateProperty->AddDelegate(Callback, Publisher);
			}

			return true;
		}

		bool UnregisterListener(FGuid ListenerId, UObject* ListenerObj, const FMulticastDelegateProperty* DelegateProperty)
		{
			if (!Nfrrlib::IsValid(ListenerObj) || !ListenerId.IsValid() || !Nfrrlib::IsValid(DelegateProperty))
			{
				return false;
			}

			FListenerEntry* Entry = Listeners.Find(ListenerId);
			if (!Nfrrlib::IsValid(Entry) || Entry->Listener.Get() != ListenerObj)
			{
				return false;
			}

			for (const FPublisherEntry& PublisherEntry : Publishers)
			{
				UObject* Publisher = PublisherEntry.Publisher.Get();
				if (Nfrrlib::IsValid(Publisher) && Entry->Callback.IsBound())
				{
					DelegateProperty->RemoveDelegate(Entry->Callback, Publisher);
				}
			}

			Listeners.Remove(ListenerId);
			return true;
		}

		bool IsEmpty()
		{
			CleanupPublishers();
			CleanupListeners();
			return Publishers.IsEmpty() && Listeners.IsEmpty();
		}

	private:
		void CleanupPublishers()
		{
			for (int32 Index = Publishers.Num() - 1; Index >= 0; --Index)
			{
				if (!Nfrrlib::IsValid(Publishers[Index].Publisher))
				{
					Publishers.RemoveAtSwap(Index);
				}
			}
		}

		void CleanupListeners()
		{
			for (auto It = Listeners.CreateIterator(); It; ++It)
			{
				if (!Nfrrlib::IsValid(It.Value().Listener))
				{
					It.RemoveCurrent();
				}
			}
		}

		void UnbindPublisherFromAllListeners(const FPublisherEntry& PublisherEntry, const FMulticastDelegateProperty* DelegateProperty)
		{
			UObject* Publisher = PublisherEntry.Publisher.Get();
			if (!Nfrrlib::IsValid(Publisher) || !Nfrrlib::IsValid(DelegateProperty))
			{
				return;
			}

			CleanupListeners();
			for (const TPair<FGuid, FListenerEntry>& Pair : Listeners)
			{
				const FListenerEntry& ListenerEntry = Pair.Value;
				if (Nfrrlib::IsValid(ListenerEntry.Listener) && ListenerEntry.Callback.IsBound())
				{
					DelegateProperty->RemoveDelegate(ListenerEntry.Callback, Publisher);
				}
			}
		}

	private:
		TArray<FPublisherEntry> Publishers;
		TMap<FGuid, FListenerEntry> Listeners;
	};

	FEventBusManager::~FEventBusManager() = default;

	void FEventBusManager::Reset()
	{
		if (!EnsureGameThread(TEXT("Reset")))
		{
			return;
		}

		Buses.Reset();
		ChannelRoutes.Reset();
	}

	bool FEventBusManager::AddListenerByChannel(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName)
	{
		return AddListenerByChannelImpl(ChannelTag, ListenerObj, FuncName);
	}

	bool FEventBusManager::RemoveListenerByChannel(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName)
	{
		return RemoveListenerByChannelImpl(ChannelTag, ListenerObj, FuncName);
	}

	bool FEventBusManager::AddPublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj)
	{
		if (!EnsureGameThread(TEXT("AddPublisherByChannel")))
		{
			return false;
		}

		if (!ChannelTag.IsValid() || !Nfrrlib::IsValid(PublisherObj))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByChannel failed: Channel=%s Publisher=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		FChannelRoute* Route = ChannelRoutes.Find(ChannelTag);
		if (!Nfrrlib::IsValid(Route))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByChannel failed: no route for Channel=%s. Use the overload with DelegatePropertyName to create one."),
				*ChannelTag.ToString());
			return false;
		}

		return AddPublisherByRoute(ChannelTag, PublisherObj, *Route);
	}

	bool FEventBusManager::AddPublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj, FName DelegatePropertyName)
	{
		if (!EnsureGameThread(TEXT("AddPublisherByChannel")))
		{
			return false;
		}

		if (!ChannelTag.IsValid() || !Nfrrlib::IsValid(PublisherObj))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByChannel failed: Channel=%s Publisher=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		FChannelRoute* ExistingRoute = ChannelRoutes.Find(ChannelTag);
		if (Nfrrlib::IsValid(ExistingRoute))
		{
			if (Nfrrlib::IsValid(ExistingRoute->RuntimeBus) &&
				!DelegatePropertyName.IsNone() &&
				ExistingRoute->DelegatePropertyName != DelegatePropertyName)
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByChannel failed: Channel=%s already mapped to delegate property [%s], requested [%s]."),
					*ChannelTag.ToString(),
					*ExistingRoute->DelegatePropertyName.ToString(),
					*DelegatePropertyName.ToString());
				return false;
			}

			return AddPublisherByRoute(ChannelTag, PublisherObj, *ExistingRoute);
		}

		FChannelRoute NewRoute;
		if (!CreateRuntimeRoute(ChannelTag, PublisherObj, DelegatePropertyName, NewRoute))
		{
			return false;
		}

		FChannelRoute& AddedRoute = ChannelRoutes.Add(ChannelTag, MoveTemp(NewRoute));
		return AddPublisherByRoute(ChannelTag, PublisherObj, AddedRoute);
	}

	bool FEventBusManager::RemovePublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj)
	{
		if (!EnsureGameThread(TEXT("RemovePublisherByChannel")))
		{
			return false;
		}

		if (!ChannelTag.IsValid() || !Nfrrlib::IsValid(PublisherObj))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemovePublisherByChannel failed: Channel=%s Publisher=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		FChannelRoute* Route = ChannelRoutes.Find(ChannelTag);
		if (!Nfrrlib::IsValid(Route))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemovePublisherByChannel failed: no route for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (Nfrrlib::IsValid(Route->RemovePublisherOp))
		{
			return Route->RemovePublisherOp(*this, PublisherObj);
		}

		if (!Nfrrlib::IsValid(Route->RuntimeBus))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemovePublisherByChannel failed: route has no runtime bus for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		const bool bRemoved = Route->RuntimeBus->UnregisterPublisher(PublisherObj, Route->DelegateProperty);
		if (bRemoved && Route->RuntimeBus->IsEmpty())
		{
			ChannelRoutes.Remove(ChannelTag);
		}
		return bRemoved;
	}

	bool FEventBusManager::AddListenerByChannelImpl(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName)
	{
		if (!EnsureGameThread(TEXT("AddListenerByChannel")))
		{
			return false;
		}

		if (!ChannelTag.IsValid() || !Nfrrlib::IsValid(ListenerObj) || FuncName.IsNone())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannel failed: Channel=%s Listener=%s Func=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*FuncName.ToString());
			return false;
		}

		FChannelRoute* Route = ChannelRoutes.Find(ChannelTag);
		if (!Nfrrlib::IsValid(Route))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannel failed: no route for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (Nfrrlib::IsValid(Route->AddListenerOp))
		{
			return Route->AddListenerOp(*this, ListenerObj, FuncName);
		}

		if (!Nfrrlib::IsValid(Route->RuntimeBus))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannel failed: route has no runtime bus for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (!Nfrrlib::IsValid(Route->DelegateProperty))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannel failed: missing runtime delegate metadata for Channel=%s Listener=%s Func=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*FuncName.ToString());
			return false;
		}

		const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
		const FScriptDelegate Callback = Detail::MakeScriptDelegate(ListenerObj, FuncName);
		if (!ListenerId.IsValid() || !Callback.IsBound())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListenerByChannel failed: invalid runtime callback for Channel=%s Listener=%s Func=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*FuncName.ToString());
			return false;
		}

		return Route->RuntimeBus->RegisterListener(ListenerId, ListenerObj, Callback, Route->DelegateProperty);
	}

	bool FEventBusManager::RemoveListenerByChannelImpl(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName)
	{
		if (!EnsureGameThread(TEXT("RemoveListenerByChannel")))
		{
			return false;
		}

		if (!ChannelTag.IsValid() || !Nfrrlib::IsValid(ListenerObj) || FuncName.IsNone())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemoveListenerByChannel failed: Channel=%s Listener=%s Func=%s"),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*FuncName.ToString());
			return false;
		}

		FChannelRoute* Route = ChannelRoutes.Find(ChannelTag);
		if (!Nfrrlib::IsValid(Route))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemoveListenerByChannel failed: no route for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (Nfrrlib::IsValid(Route->RemoveListenerOp))
		{
			return Route->RemoveListenerOp(*this, ListenerObj, FuncName);
		}

		if (!Nfrrlib::IsValid(Route->RuntimeBus))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemoveListenerByChannel failed: route has no runtime bus for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		const FGuid ListenerId = Detail::MakeListenerId(ListenerObj, FuncName);
		const bool bRemoved = Route->RuntimeBus->UnregisterListener(ListenerId, ListenerObj, Route->DelegateProperty);
		if (bRemoved && Route->RuntimeBus->IsEmpty())
		{
			ChannelRoutes.Remove(ChannelTag);
		}
		return bRemoved;
	}

	bool FEventBusManager::CreateRuntimeRoute(const FGameplayTag& ChannelTag, UObject* PublisherObj, FName DelegatePropertyName, FChannelRoute& OutRoute)
	{
		if (DelegatePropertyName.IsNone())
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("CreateRuntimeRoute failed: DelegatePropertyName is None for Channel=%s"),
				*ChannelTag.ToString());
			return false;
		}

		const FMulticastDelegateProperty* DelegateProperty = FindMulticastDelegateProperty(PublisherObj, DelegatePropertyName);
		if (!Nfrrlib::IsValid(DelegateProperty))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("CreateRuntimeRoute failed: delegate property [%s] not found on Publisher=%s"),
				*DelegatePropertyName.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		OutRoute.TagTypeToken = nullptr;
		OutRoute.bOwnsPublisherDelegates = false;
		OutRoute.AddPublisherOp = nullptr;
		OutRoute.RemovePublisherOp = nullptr;
		OutRoute.AddListenerOp = nullptr;
		OutRoute.RemoveListenerOp = nullptr;
		OutRoute.DelegatePropertyName = DelegatePropertyName;
		OutRoute.PublisherClass = PublisherObj->GetClass();
		OutRoute.DelegateProperty = DelegateProperty;
		OutRoute.RuntimeBus = MakeShared<FRuntimeChannelBus>();
		return true;
	}

	bool FEventBusManager::AddPublisherByRoute(const FGameplayTag& ChannelTag, UObject* PublisherObj, FChannelRoute& Route)
	{
		if (Nfrrlib::IsValid(Route.AddPublisherOp))
		{
			return Route.AddPublisherOp(*this, PublisherObj);
		}

		if (!Nfrrlib::IsValid(Route.RuntimeBus))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByRoute failed: route has no runtime bus for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (!Nfrrlib::IsValid(Route.DelegateProperty) || !Nfrrlib::IsValid(Route.PublisherClass))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByRoute failed: invalid runtime route metadata for Channel=%s"), *ChannelTag.ToString());
			return false;
		}

		if (!PublisherObj->IsA(Route.PublisherClass))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisherByRoute failed: Publisher=%s is not compatible with route class=%s for Channel=%s"),
				*GetNameSafe(PublisherObj),
				*GetNameSafe(Route.PublisherClass),
				*ChannelTag.ToString());
			return false;
		}

		return Route.RuntimeBus->RegisterPublisher(PublisherObj, Route.DelegateProperty);
	}

	const FMulticastDelegateProperty* FEventBusManager::FindMulticastDelegateProperty(UObject* PublisherObj, FName DelegatePropertyName) const
	{
		if (!Nfrrlib::IsValid(PublisherObj) || DelegatePropertyName.IsNone())
		{
			return nullptr;
		}

		const FProperty* Property = PublisherObj->GetClass()->FindPropertyByName(DelegatePropertyName);
		return CastField<const FMulticastDelegateProperty>(Property);
	}

	bool FEventBusManager::EnsureGameThread(const TCHAR* Context) const
	{
		if (IsInGameThread())
		{
			return true;
		}

		UE_LOG(LogNFLEventBus, Warning, TEXT("EventBusManager: %s must be called on the Game Thread."), Context);
		return false;
	}
} // namespace Nfrrlib
