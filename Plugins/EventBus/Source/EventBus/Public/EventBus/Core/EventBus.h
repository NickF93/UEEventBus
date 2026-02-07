#pragma once

#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "Templates/UniquePtr.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBusErrors.h"
#include "EventBus/Core/EventBusTypes.h"

namespace Nfrrlib::EventBus::Private
{
	class FEventChannelState;

	/**
	 * @brief Custom deleter used to keep channel state type private and forward-declared in public headers.
	 */
	struct FEventChannelStateDeleter
	{
		void operator()(FEventChannelState* ChannelState) const;
	};
}

/**
 * @brief Log category for EventBus diagnostics.
 */
EVENTBUS_API DECLARE_LOG_CATEGORY_EXTERN(LogNFLEventBus, Log, All);

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Runtime event bus that orchestrates channel registration and bindings.
	 *
	 * API Contract:
	 * - Register/Unregister channel explicitly.
	 * - Add/Remove publisher by channel + publisher + delegate binding.
	 * - Add/Remove listener by channel + listener + function binding.
	 *
	 * Threading:
	 * - Not thread-safe.
	 * - All operations must run on the Game Thread.
	 */
	class EVENTBUS_API FEventBus final
	{
	public:
		FEventBus() = default;
		~FEventBus();

		FEventBus(const FEventBus&) = delete;
		FEventBus& operator=(const FEventBus&) = delete;

		/** @brief Registers a channel and its ownership policy. Idempotent when policy matches existing route. */
		NFL_EVENTBUS_NODISCARD bool RegisterChannel(const FChannelRegistration& Registration);
		/** @brief Unregisters a channel and unbinds all tracked callbacks under it. */
		NFL_EVENTBUS_NODISCARD bool UnregisterChannel(const FGameplayTag& ChannelTag);
		/** @brief Returns true when channel is currently registered. */
		NFL_EVENTBUS_NODISCARD bool IsChannelRegistered(const FGameplayTag& ChannelTag) const;

		/** @brief Adds or updates publisher delegate binding for one channel. */
		NFL_EVENTBUS_NODISCARD bool AddPublisher(const FGameplayTag& ChannelTag, UObject* PublisherObj, const FPublisherBinding& Binding);
		/** @brief Removes publisher delegate binding for one channel. */
		NFL_EVENTBUS_NODISCARD bool RemovePublisher(const FGameplayTag& ChannelTag, UObject* PublisherObj);

		/** @brief Adds or updates one listener function binding for one channel. */
		NFL_EVENTBUS_NODISCARD bool AddListener(const FGameplayTag& ChannelTag, UObject* ListenerObj, const FListenerBinding& Binding);
		/** @brief Removes one listener function binding for one channel. */
		NFL_EVENTBUS_NODISCARD bool RemoveListener(const FGameplayTag& ChannelTag, UObject* ListenerObj, const FListenerBinding& Binding);

		/** @brief Clears every channel and unbinds all tracked callbacks. */
		void Reset();

	private:
		using FChannelStatePtr = TUniquePtr<Private::FEventChannelState, Private::FEventChannelStateDeleter>;

		/**
		 * @brief Returns mutable state for a channel, or nullptr when the channel is not registered.
		 */
		Private::FEventChannelState* FindChannelState(const FGameplayTag& ChannelTag);

		/**
		 * @brief Returns immutable state for a channel, or nullptr when the channel is not registered.
		 */
		const Private::FEventChannelState* FindChannelState(const FGameplayTag& ChannelTag) const;

	private:
		TMap<FGameplayTag, FChannelStatePtr> Channels;
	};
} // namespace Nfrrlib::EventBus
