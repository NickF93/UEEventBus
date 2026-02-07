#pragma once

/**
 * @file EventBus.h
 * @brief Event bus for binding UObject publishers to UObject listeners via dynamic delegates.
 *
 * The bus tracks publishers and listeners and keeps multicast bindings in sync.
 * Listener ids are deterministic, so callers do not need to store them for removal.
 *
 * Threading: this type is NOT thread-safe. Use only on the Game Thread.
 */

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Misc/Guid.h"
#include "UObject/Object.h"
#include "UObject/ScriptDelegates.h"
#include "EventBus/Utility/Util.h"

#include "Templates/IsPointer.h"
#include "Templates/UnrealTypeTraits.h"
#include "Traits/MemberFunctionPtrOuter.h"

/**
 * The <type_traits> header is intentionally used for compile-time transformations and checks.
 *
 * Rationale:
 * - UE 5.5 deprecates several legacy trait aliases in favor of std equivalents.
 * - This file uses std::remove_cv_t and related std traits to avoid deprecated APIs.
 *
 * UE deprecation context:
 * - UnrealTypeTraits.h deprecates TIsSame with:
 *   "TIsSame has been deprecated, please use std::is_same instead."
 * - UE 5.5 warnings deprecate TRemoveCV with:
 *   "TRemoveCV has been deprecated, please use std::remove_cv_t instead."
 *
 * Reference:
 * https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine
 */
#include <type_traits>

/** Log category for EventBus diagnostics. */
EVENTBUS_API DECLARE_LOG_CATEGORY_EXTERN(LogNFLEventBus, Log, All);

/**
 * @def NFL_VALIDATED_FUNC_FNAME
 * @brief Compile-time validation of a member function pointer, returning its FName.
 *
 * Ensures:
 * - UserObject is a raw UObject pointer.
 * - FuncName is a member function pointer (&Class::Function).
 * - FuncName belongs to UserObject's class (or a base class).
 */
#define NFL_VALIDATED_FUNC_FNAME(UserObject, FuncName)														\
Nfrrlib::Detail::ValidateListenerFunc<decltype(UserObject), decltype(FuncName)>(								\
STATIC_FUNCTION_FNAME(TEXT(#FuncName)))

/**
 * @def NFLAddUniqueDynamic
 * @brief Register a listener with an auto-generated deterministic id.
 */
#define NFLAddUniqueDynamic(UserObject, FuncName)															\
RegisterListener(																							\
Nfrrlib::Detail::MakeListenerId((UserObject), NFL_VALIDATED_FUNC_FNAME(UserObject, FuncName)),				\
(UserObject),																								\
Nfrrlib::Detail::MakeScriptDelegate((UserObject), NFL_VALIDATED_FUNC_FNAME(UserObject, FuncName))				\
)

/**
 * @def NFLRemoveDynamic
 * @brief Unregister a listener using the same deterministic id as NFLAddUniqueDynamic.
 */
#define NFLRemoveDynamic(UserObject, FuncName)																\
UnregisterListener(																							\
Nfrrlib::Detail::MakeListenerId((UserObject), NFL_VALIDATED_FUNC_FNAME(UserObject, FuncName)),				\
(UserObject)																								\
)

/**
 * @def NFLAddUniqueDynamicId
 * @brief Register a listener with a caller-provided id.
 */
#define NFLAddUniqueDynamicId(ListenerId, UserObject, FuncName)											\
RegisterListener(																							\
(ListenerId),																								\
(UserObject),																								\
Nfrrlib::Detail::MakeScriptDelegate((UserObject), NFL_VALIDATED_FUNC_FNAME(UserObject, FuncName))				\
)

namespace Nfrrlib
{
	namespace Detail
	{
		/** Strip reference qualifier from a type. */
		template <typename TObject>
		using TObjectNoRef = typename TRemoveReference<TObject>::Type;

		/** Strip const/volatile qualifiers from a type. */
		template <typename TObject>
		using TObjectNoCV = std::remove_cv_t<TObjectNoRef<TObject>>;

		/** Strip pointer qualifier from a type. */
		template <typename TObject>
		using TObjectNoPtr = typename TRemovePointer<TObjectNoCV<TObject>>::Type;

		/** Strip ref + cv from a type. */
		template <typename T>
		using TNoCVRef = std::remove_cv_t<typename TRemoveReference<T>::Type>;

		/**
		 * @brief Detect member function pointers using UE's TMemberFunctionPtrOuter_T.
		 */
		template <typename T>
		concept CMemberFunctionPointer = requires { typename TMemberFunctionPtrOuter_T<TNoCVRef<T>>; };

		/**
		 * @brief Validate a listener function pointer at compile time and return its FName.
		 */
		template <typename TObject, typename TFunc>
		FORCEINLINE FName ValidateListenerFunc(const FName InFuncName)
		{
			static_assert(TIsPointer<TObjectNoRef<TObject>>::Value,
				"UserObject must be a raw UObject pointer.");
			static_assert(TIsDerivedFrom<TObjectNoPtr<TObject>, UObject>::Value,
				"UserObject must be a UObject-derived pointer.");
			static_assert(CMemberFunctionPointer<TFunc>,
				"FuncName must be a member function pointer like &Class::Function.");

			if constexpr (CMemberFunctionPointer<TFunc>)
			{
				using FuncClass = TMemberFunctionPtrOuter_T<TNoCVRef<TFunc>>;
				static_assert(TIsDerivedFrom<TObjectNoPtr<TObject>, FuncClass>::Value,
					"FuncName must belong to UserObject class.");
			}

			return InFuncName;
		}

		/**
		 * @brief Build a deterministic listener id from object + function name.
		 */
		FORCEINLINE FGuid MakeListenerId(const UObject* Obj, const FName Func)
		{
			if (!Nfrrlib::IsValid(Obj) || Func.IsNone())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("MakeListenerId failed: Obj=%s Func=%s"),
					*GetNameSafe(Obj),
					*Func.ToString());
				return FGuid();
			}

			// Fixed seed allows deterministic ids and leaves room for domain separation later.
			constexpr uint64 Seed = 0x000000u;
			const FString Key = FString::Printf(TEXT("%s::%s"), *Obj->GetPathName(), *Func.ToString());
			return FGuid::NewDeterministicGuid(Key, Seed);
		}
		
		/**
		 * @brief Create a pre-bound script delegate to a UFUNCTION.
		 */
		FORCEINLINE FScriptDelegate MakeScriptDelegate(UObject* Obj, const FName Func)
		{
			if (!Nfrrlib::IsValid(Obj) || Func.IsNone())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("MakeScriptDelegate failed: Obj=%s Func=%s"),
					*GetNameSafe(Obj),
					*Func.ToString());
				return FScriptDelegate();
			}

			FScriptDelegate Delegate;
			Delegate.BindUFunction(Obj, Func);

			if (!Delegate.IsBound())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("MakeScriptDelegate bind failed: Obj=%s Func=%s"),
					*GetNameSafe(Obj),
					*Func.ToString());
			}

			return Delegate;
		}
	} // namespace Detail
	
	/** Concept: type derives from UObject. */
	template <typename TObject>
	concept CUObjectDerived = TIsDerivedFrom<TObject, UObject>::Value;
	
	/**
	 * @class TEventBus
	 * @brief Event bus mapping publishers to listeners for a specific event tag.
	 *
	 * @tparam TEventTag               Tag type defining the delegate (TEventTag::FDelegate).
	 * @tparam bOwnsPublisherDelegates If true, the bus owns publisher delegates and may remove
	 *                                 all bindings for a listener object (RemoveAll). If false,
	 *                                 only the specific callback is removed.
	 */
	template <typename TEventTag, bool bOwnsPublisherDelegates = false>
	class TEventBus final
	{
	public:
		
		/** Delegate type defined by the event tag. */
		using FDelegate = typename TEventTag::FDelegate;
		/** Publisher id type for event routing. */
		using FPublisherId = FGameplayTag;
		
		/**
		 * @brief Publisher entry stored per PublisherId.
		 *
		 * Publisher: weak pointer to the object owning the multicast delegate.
		 * GetDelegate: accessor returning the delegate reference from the publisher.
		 */
		struct FPublisherEntry
		{
			TWeakObjectPtr<UObject> Publisher;
			TFunction<FDelegate&(UObject*)> GetDelegate; // returns ref to publisher's delegate
		};
		
		/**
		 * @brief Listener entry stored per ListenerId.
		 *
		 * Listener: weak pointer to the listener object.
		 * Callback: bound script delegate (object + UFUNCTION name).
		 */
		struct FListenerEntry
		{
			TWeakObjectPtr<UObject> Listener;
			FScriptDelegate Callback; // UObject + UFUNCTION name
		};
		
		/**
		 * @brief Register a publisher for a given PublisherId.
		 *
		 * If a publisher with the same id and object already exists, it is updated and re-bound
		 * to all existing listeners. If a new publisher is added, all current listeners are bound
		 * to it.
		 *
		 * @return true if the publisher registration was applied.
		 */
		template <CUObjectDerived TPublisher>
		bool RegisterPublisher(const FPublisherId& PublisherId, TPublisher* PublisherPtr, FDelegate TPublisher::* DelegateMember)
		{
			if (!EnsureGameThread(TEXT("RegisterPublisher")))
			{
				return false;
			}

			if (!Nfrrlib::IsValid(PublisherPtr) || !PublisherId.IsValid())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("RegisterPublisher failed: Publisher=%s Id=%s"),
					*GetNameSafe(PublisherPtr),
					*PublisherId.ToString());
				return false;
			}

			FPublisherEntry Entry;
			Entry.Publisher = PublisherPtr;
			
			// Capture the delegate member pointer for later delegate access.
			Entry.GetDelegate = [DelegateMember](UObject* Obj) -> FDelegate&
			{
				auto* Typed = static_cast<TPublisher*>(Obj);
				return Typed->*DelegateMember;
			};
			
			// Support multiple publishers per id; update if the same object re-registers.
			TArray<FPublisherEntry>& Bucket = Publishers.FindOrAdd(PublisherId);
			int32 ExistingIndex = Bucket.IndexOfByPredicate([PublisherPtr](const FPublisherEntry& Existing)
			{
				return Existing.Publisher.Get() == PublisherPtr;
			});

			if (ExistingIndex != INDEX_NONE)
			{
				// Update existing entry and clean its bindings before re-adding.
				UnbindPublisherFromAllListeners(Bucket[ExistingIndex]);
				Bucket[ExistingIndex] = MoveTemp(Entry);
			}
			else
			{
				Bucket.Add(MoveTemp(Entry));
				ExistingIndex = Bucket.Num() - 1;
			}

			FPublisherEntry& NewEntry = Bucket[ExistingIndex];
			FDelegate& PubDelegate = NewEntry.GetDelegate(PublisherPtr);

			// Bind all existing listeners to this publisher.
			CleanupListeners();
			for (const auto& Pair : Listeners)
			{
				const FListenerEntry& LEntry = Pair.Value;
				UObject* ListenerObj = LEntry.Listener.Get();

				if (!Nfrrlib::IsValid(ListenerObj) || !LEntry.Callback.IsBound())
				{
					continue;
				}

				// Avoid duplicates for this listener on this publisher.
				if constexpr (bOwnsPublisherDelegates)
				{
					// Bus owns delegate; remove all bindings from that listener object.
					PubDelegate.RemoveAll(ListenerObj);
				}
				else
				{
					// Bus does not own delegate; remove only its specific callback.
					PubDelegate.Remove(LEntry.Callback);
				}

				// Add pre-bound dynamic callback (object + function name).
				PubDelegate.AddUnique(LEntry.Callback);
			}

			return true;
		}
		
		/**
		 * @brief Unregister a publisher by instance pointer.
		 *
		 * @return true if at least one publisher entry was removed.
		 */
		bool UnregisterPublisher(UObject* PublisherObj)
		{
			if (!EnsureGameThread(TEXT("UnregisterPublisher")))
			{
				return false;
			}

			if (!Nfrrlib::IsValid(PublisherObj))
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("UnregisterPublisher failed: invalid PublisherObj"));
				return false;
			}

			bool bRemovedAny = false;
			for (auto It = Publishers.CreateIterator(); It; ++It)
			{
				TArray<FPublisherEntry>& Bucket = It.Value();
				for (int32 Index = Bucket.Num() - 1; Index >= 0; --Index)
				{
					if (Bucket[Index].Publisher.Get() == PublisherObj)
					{
						UnbindPublisherFromAllListeners(Bucket[Index]);
						Bucket.RemoveAtSwap(Index);
						bRemovedAny = true;
					}
				}

				if (Bucket.IsEmpty())
				{
					It.RemoveCurrent();
				}
			}

			return bRemovedAny;
		}
		
		/**
		 * @brief Unregister all publishers for a given PublisherId.
		 *
		 * @return true if at least one publisher was removed.
		 */
		bool UnregisterPublisherById(const FPublisherId& PublisherId)
		{
			if (!EnsureGameThread(TEXT("UnregisterPublisherById")))
			{
				return false;
			}

			if (!PublisherId.IsValid())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("UnregisterPublisherById failed: invalid PublisherId"));
				return false;
			}

			TArray<FPublisherEntry>* Bucket = Publishers.Find(PublisherId);
			if (Nfrrlib::IsValid(Bucket))
			{
				for (FPublisherEntry& Entry : *Bucket)
				{
					UnbindPublisherFromAllListeners(Entry);
				}

				Publishers.Remove(PublisherId);
				return true;
			}

			UE_LOG(LogNFLEventBus, Verbose, TEXT("UnregisterPublisherById: no publishers for Id=%s"), *PublisherId.ToString());
			return false;
		}
		
		/**
		 * @brief Register a listener for all current and future publishers.
		 *
		 * @return true if listener registration was applied.
		 */
		template <CUObjectDerived TListener>
		bool RegisterListener(const FGuid ListenerId, TListener* ListenerObj, const FScriptDelegate& Callback)
		{
			if (!EnsureGameThread(TEXT("RegisterListener")))
			{
				return false;
			}

			if (!Nfrrlib::IsValid(ListenerObj) || !ListenerId.IsValid() || !Callback.IsBound())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("RegisterListener failed: Listener=%s Id=%s CallbackBound=%d"),
					*GetNameSafe(ListenerObj),
					*ListenerId.ToString(EGuidFormats::DigitsWithHyphensLower),
					Callback.IsBound());
				return false;
			}

			// Ensure the callback targets the same object being registered.
			if (Callback.GetUObject() != ListenerObj)
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("RegisterListener failed: callback target mismatch. Listener=%s CallbackObj=%s"),
					*GetNameSafe(ListenerObj),
					*GetNameSafe(Callback.GetUObject()));
				return false;
			}

			FListenerEntry* OldEntry = Listeners.Find(ListenerId);
			if (Nfrrlib::IsValid(OldEntry))
			{
				TWeakObjectPtr<UObject> OldListenerObj = OldEntry->Listener;
				if (Nfrrlib::IsValid(OldListenerObj))
				{
					// Remove old callback from all publishers before replacing it.
					CleanupPublishers();

					ForEachPublisherEntry([&OldEntry](FPublisherEntry& PubEntry)
					{
						UObject* PubObj = PubEntry.Publisher.Get();
						if (!Nfrrlib::IsValid(PubObj))
						{
							return;
						}

						FDelegate& PubDelegate = PubEntry.GetDelegate(PubObj);
						if (OldEntry->Callback.IsBound())
						{
							PubDelegate.Remove(OldEntry->Callback);
						}
					});
				}
			}

			FListenerEntry Entry;
			Entry.Listener = ListenerObj;
			Entry.Callback = Callback;
			Listeners.Add(ListenerId, MoveTemp(Entry));

			// Bind this new listener to all existing publishers.
			CleanupPublishers();
			ForEachPublisherEntry([&Callback](FPublisherEntry& PubEntry)
			{
				UObject* PubObj = PubEntry.Publisher.Get();
				if (!Nfrrlib::IsValid(PubObj))
				{
					return;
				}

				FDelegate& PubDelegate = PubEntry.GetDelegate(PubObj);
				PubDelegate.Remove(Callback);
				PubDelegate.AddUnique(Callback);
			});

			return true;
		}

		/**
		 * @brief Unregister a listener by id and object.
		 *
		 * @return true if a matching listener entry was removed.
		 */
		bool UnregisterListener(const FGuid ListenerId, UObject* ListenerObj)
		{
			if (!EnsureGameThread(TEXT("UnregisterListener")))
			{
				return false;
			}

			if (!Nfrrlib::IsValid(ListenerObj) || !ListenerId.IsValid())
			{
				UE_LOG(LogNFLEventBus, Warning, TEXT("UnregisterListener failed: Listener=%s Id=%s"),
					*GetNameSafe(ListenerObj),
					*ListenerId.ToString(EGuidFormats::DigitsWithHyphensLower));
				return false;
			}

			FListenerEntry* Entry = Listeners.Find(ListenerId);
			if (!Nfrrlib::IsValid(Entry) || Entry->Listener.Get() != ListenerObj)
			{
				UE_LOG(LogNFLEventBus, Verbose, TEXT("UnregisterListener: no matching entry. Listener=%s Id=%s"),
					*GetNameSafe(ListenerObj),
					*ListenerId.ToString(EGuidFormats::DigitsWithHyphensLower));
				return false;
			}

			CleanupPublishers();
			ForEachPublisherEntry([Entry](FPublisherEntry& PubEntry)
			{
				UObject* PubObj = PubEntry.Publisher.Get();
				if (!Nfrrlib::IsValid(PubObj))
				{
					return;
				}

				FDelegate& PubDelegate = PubEntry.GetDelegate(PubObj);
				if (Entry->Callback.IsBound())
				{
					PubDelegate.Remove(Entry->Callback);
				}
			});

			Listeners.Remove(ListenerId);
			return true;
		}

	private:
		FORCEINLINE bool EnsureGameThread(const TCHAR* Context) const
		{
			if (IsInGameThread())
			{
				return true;
			}

			UE_LOG(LogNFLEventBus, Warning, TEXT("EventBus: %s must be called on the Game Thread."), Context);
			return false;
		}

		/**
		 * @brief Iterate all publisher entries across all buckets.
		 */
		template <typename TFunc>
		FORCEINLINE void ForEachPublisherEntry(TFunc&& Func)
		{
			for (auto& Pair : Publishers)
			{
				for (FPublisherEntry& Entry : Pair.Value)
				{
					Func(Entry);
				}
			}
		}

		/**
		 * @brief Remove invalid publisher entries from all buckets.
		 */
		FORCEINLINE void CleanupPublishers()
		{
			for (auto It = Publishers.CreateIterator(); It; ++It)
			{
				TArray<FPublisherEntry>& Bucket = It.Value();
				for (int32 Index = Bucket.Num() - 1; Index >= 0; --Index)
				{
					if (!Nfrrlib::IsValid(Bucket[Index].Publisher))
					{
						Bucket.RemoveAtSwap(Index);
					}
				}

				if (Bucket.IsEmpty())
				{
					It.RemoveCurrent();
				}
			}
		}

		/**
		 * @brief Remove invalid listener entries.
		 */
		FORCEINLINE void CleanupListeners()
		{
			for (auto It = Listeners.CreateIterator(); It; ++It)
			{
				if (!Nfrrlib::IsValid(It.Value().Listener))
				{
					It.RemoveCurrent();
				}
			}
		}

		/**
		 * @brief Remove all listener bindings from a specific publisher entry.
		 */
		FORCEINLINE void UnbindPublisherFromAllListeners(FPublisherEntry& PublisherEntry)
		{
			UObject* PubObj = PublisherEntry.Publisher.Get();
			if (!Nfrrlib::IsValid(PubObj))
			{
				return;
			}

			FDelegate& PubDelegate = PublisherEntry.GetDelegate(PubObj);

			CleanupListeners();
			for (const auto& Pair : Listeners)
			{
				const FListenerEntry& LEntry = Pair.Value;
				TWeakObjectPtr<UObject> ListenerObj = LEntry.Listener;
				if (Nfrrlib::IsValid(ListenerObj) && LEntry.Callback.IsBound())
				{
					PubDelegate.Remove(LEntry.Callback);
				}
			}
		}

		/** Publishers grouped by PublisherId (supports multiple publishers per id). */
		TMap<FPublisherId, TArray<FPublisherEntry>> Publishers;

		/** Listeners indexed by deterministic id. */
		TMap<FGuid, FListenerEntry> Listeners;
	};
} // namespace Nfrrlib

