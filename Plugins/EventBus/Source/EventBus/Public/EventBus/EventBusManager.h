/**
 * @file EventBusManager.h
 * @brief Registry/factory for typed event buses and runtime channel routes.
 *
 * Public API:
 *  - AddPublisher<TEventTag>()
 *  - RemovePublisher<TEventTag>()
 *  - AddListener<TEventTag>()
 *  - RemoveListener<TEventTag>()
 *  - RegisterChannel<TEventTag>()
 *  - AddPublisherByChannel()
 *  - RemovePublisherByChannel()
 *  - AddListenerByChannel()
 *  - RemoveListenerByChannel()
 *
 * Notes:
 * - Not thread-safe. Use only on the Game Thread.
 * - Uses type-erasure: stores one typed bus per (EventTag, bOwnsPublisherDelegates).
 * - Maintains channel routes for both typed tags and runtime-reflection channels.
 */
#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "Templates/SharedPointer.h"
#include "Templates/TypeHash.h"
#include "Templates/UniquePtr.h"

/**
 * The C++20 <concepts> header is intentionally used here.
 *
 * Rationale:
 * - UE 5.5 does not provide first-class concept equivalents for std::same_as/std::convertible_to.
 * - CEventTag uses std::same_as and std::convertible_to to express a strict compile-time contract.
 * - This keeps template errors localized and readable, and avoids duplicated SFINAE boilerplate.
 */
#include <concepts>

#include "EventBus/EventBus.h"
#include "EventBus/EventTags.h"

class FMulticastDelegateProperty;

namespace Nfrrlib
{
	/**
	 * @brief Concept for a valid EventTag contract used by TEventBus and FEventBusManager.
	 */
	template <typename TEventTag>
	concept CEventTag = requires(typename TEventTag::PublisherType* PublisherPtr)
	{
		typename TEventTag::FDelegate;
		typename TEventTag::PublisherType;
		{ TEventTag::GetPublisherId() } -> std::convertible_to<FGameplayTag>;
		{ PublisherPtr->*TEventTag::DelegateMember } -> std::same_as<typename TEventTag::FDelegate&>;
	};

	class EVENTBUS_API FEventBusManager final
	{
	public:
		FEventBusManager() = default;
		~FEventBusManager();

		FEventBusManager(const FEventBusManager&) = delete;
		FEventBusManager& operator=(const FEventBusManager&) = delete;

		/** Clears all cached buses and channel routes. */
		void Reset();

		/** Registers a publisher into the bus identified by TEventTag. */
		template <CEventTag TEventTag, bool bOwnsPublisherDelegates = false>
		FORCEINLINE void AddPublisher(typename TEventTag::PublisherType* PublisherObj);

		/** Unregisters a publisher from the bus identified by TEventTag. */
		template <CEventTag TEventTag, bool bOwnsPublisherDelegates = false>
		FORCEINLINE void RemovePublisher(UObject* PublisherObj);

		/** Registers a listener callback by function name into the bus identified by TEventTag. */
		template <CEventTag TEventTag, bool bOwnsPublisherDelegates = false, CUObjectDerived TListener>
		FORCEINLINE void AddListener(TListener* ListenerObj, FName FuncName);

		/** Unregisters a listener callback by function name from the bus identified by TEventTag. */
		template <CEventTag TEventTag, bool bOwnsPublisherDelegates = false, CUObjectDerived TListener>
		FORCEINLINE void RemoveListener(TListener* ListenerObj, FName FuncName);

		/** Registers a listener by gameplay channel using typed route lookup. */
		template <CUObjectDerived TListener>
		FORCEINLINE bool AddListenerByChannel(const FGameplayTag& ChannelTag, TListener* ListenerObj, FName FuncName);

		/** Unregisters a listener by gameplay channel using typed route lookup. */
		template <CUObjectDerived TListener>
		FORCEINLINE bool RemoveListenerByChannel(const FGameplayTag& ChannelTag, TListener* ListenerObj, FName FuncName);

		/** Registers a listener by gameplay channel using runtime UObject input. */
		bool AddListenerByChannel(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName);

		/** Unregisters a listener by gameplay channel using runtime UObject input. */
		bool RemoveListenerByChannel(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName);

		/** Registers a publisher by gameplay channel using an existing route. */
		bool AddPublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj);

		/** Registers a publisher by gameplay channel and lazily creates the route via reflection. */
		bool AddPublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj, FName DelegatePropertyName);

		/** Unregisters a publisher by gameplay channel. */
		bool RemovePublisherByChannel(const FGameplayTag& ChannelTag, UObject* PublisherObj);

		/** Registers channel routing metadata for a typed event tag. */
		template <CEventTag TEventTag, bool bOwnsPublisherDelegates = false>
		FORCEINLINE bool RegisterChannel();

	private:
		using FChannelPublisherOp = bool (*)(FEventBusManager&, UObject*);
		using FChannelListenerOp = bool (*)(FEventBusManager&, UObject*, FName);

		class FRuntimeChannelBus;

		struct FChannelRoute final
		{
			const void* TagTypeToken = nullptr;
			bool bOwnsPublisherDelegates = false;
			FChannelPublisherOp AddPublisherOp = nullptr;
			FChannelPublisherOp RemovePublisherOp = nullptr;
			FChannelListenerOp AddListenerOp = nullptr;
			FChannelListenerOp RemoveListenerOp = nullptr;
			FName DelegatePropertyName = NAME_None;
			UClass* PublisherClass = nullptr;
			const FMulticastDelegateProperty* DelegateProperty = nullptr;
			TSharedPtr<FRuntimeChannelBus> RuntimeBus;

			FChannelRoute() = default;
			~FChannelRoute() = default;
			FChannelRoute(FChannelRoute&&) noexcept = default;
			FChannelRoute& operator=(FChannelRoute&&) noexcept = default;
			FChannelRoute(const FChannelRoute&) = delete;
			FChannelRoute& operator=(const FChannelRoute&) = delete;
		};

		struct FBusKey final
		{
			const void* TagTypeToken = nullptr;
			bool bOwnsPublisherDelegates = false;

			FORCEINLINE bool operator==(const FBusKey& Other) const = default;

			friend FORCEINLINE uint32 GetTypeHash(const FBusKey& Key)
			{
				return HashCombineFast(GetTypeHash(Key.TagTypeToken), GetTypeHash(Key.bOwnsPublisherDelegates));
			}
		};

		struct IBusHolder
		{
			virtual ~IBusHolder() = default;
		};

		template <typename TEventTag, bool bOwnsPublisherDelegates>
		struct TBusHolder final : IBusHolder
		{
			TEventBus<TEventTag, bOwnsPublisherDelegates> Bus;
		};

		template <typename TEventTag>
		FORCEINLINE static const void* GetTagTypeToken();

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE static bool AddPublisherByChannelOp(FEventBusManager& Manager, UObject* PublisherObj);

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE static bool RemovePublisherByChannelOp(FEventBusManager& Manager, UObject* PublisherObj);

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE static bool AddListenerByChannelOp(FEventBusManager& Manager, UObject* ListenerObj, FName FuncName);

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE static bool RemoveListenerByChannelOp(FEventBusManager& Manager, UObject* ListenerObj, FName FuncName);

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE bool EnsureChannelRoute();

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE FBusKey MakeKey() const;

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE TEventBus<TEventTag, bOwnsPublisherDelegates>& GetOrCreateBus();

		template <CEventTag TEventTag, bool bOwnsPublisherDelegates>
		FORCEINLINE TEventBus<TEventTag, bOwnsPublisherDelegates>* GetBusIfExists();

		bool AddListenerByChannelImpl(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName);
		bool RemoveListenerByChannelImpl(const FGameplayTag& ChannelTag, UObject* ListenerObj, FName FuncName);
		bool CreateRuntimeRoute(const FGameplayTag& ChannelTag, UObject* PublisherObj, FName DelegatePropertyName, FChannelRoute& OutRoute);
		bool AddPublisherByRoute(const FGameplayTag& ChannelTag, UObject* PublisherObj, FChannelRoute& Route);
		const FMulticastDelegateProperty* FindMulticastDelegateProperty(UObject* PublisherObj, FName DelegatePropertyName) const;
		bool EnsureGameThread(const TCHAR* Context) const;

	private:
		TMap<FBusKey, TUniquePtr<IBusHolder>> Buses;
		TMap<FGameplayTag, FChannelRoute> ChannelRoutes;
	};
} // namespace Nfrrlib

#include "EventBus/Detail/EventBusManager.inl"

/**
 * @def NFL_EVENTBUS_ADD_LISTENER
 * @brief Adds a listener using compile-time validated function pointer -> FName conversion.
 */
#define NFL_EVENTBUS_ADD_LISTENER(Manager, EventTagType, ListenerObj, FuncPtr) \
	(Manager).AddListener<EventTagType>( \
		(ListenerObj), \
		NFL_VALIDATED_FUNC_FNAME((ListenerObj), FuncPtr) \
	)

/**
 * @def NFL_EVENTBUS_REMOVE_LISTENER
 * @brief Removes a listener using compile-time validated function pointer -> FName conversion.
 */
#define NFL_EVENTBUS_REMOVE_LISTENER(Manager, EventTagType, ListenerObj, FuncPtr) \
	(Manager).RemoveListener<EventTagType>( \
		(ListenerObj), \
		NFL_VALIDATED_FUNC_FNAME((ListenerObj), FuncPtr) \
	)

/**
 * @def NFL_EVENTBUS_ADD_LISTENER_BY_CHANNEL
 * @brief Adds a listener by gameplay channel using compile-time validated function pointer -> FName conversion.
 */
#define NFL_EVENTBUS_ADD_LISTENER_BY_CHANNEL(Manager, ChannelTag, ListenerObj, FuncPtr) \
	(Manager).AddListenerByChannel( \
		(ChannelTag), \
		(ListenerObj), \
		NFL_VALIDATED_FUNC_FNAME((ListenerObj), FuncPtr) \
	)

/**
 * @def NFL_EVENTBUS_REMOVE_LISTENER_BY_CHANNEL
 * @brief Removes a listener by gameplay channel using compile-time validated function pointer -> FName conversion.
 */
#define NFL_EVENTBUS_REMOVE_LISTENER_BY_CHANNEL(Manager, ChannelTag, ListenerObj, FuncPtr) \
	(Manager).RemoveListenerByChannel( \
		(ChannelTag), \
		(ListenerObj), \
		NFL_VALIDATED_FUNC_FNAME((ListenerObj), FuncPtr) \
	)
