#pragma once

#include "CoreMinimal.h"
#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBusTypes.h"

class FMulticastDelegateProperty;

namespace Nfrrlib::EventBus::Private
{
	/**
	 * @brief Mutable state for a single channel.
	 */
	class FEventChannelState final
	{
	public:
		/** @brief Creates channel state with fixed ownership policy. */
		explicit FEventChannelState(bool bInOwnsPublisherDelegates);

		/** @brief Checks whether requested ownership policy matches registered policy. */
		NFL_EVENTBUS_NODISCARD bool MatchesOwnershipPolicy(bool bInOwnsPublisherDelegates) const;
		/** @brief Returns channel ownership policy configured during registration. */
		NFL_EVENTBUS_NODISCARD bool OwnsPublisherDelegates() const;

		/** @brief Registers or updates a publisher for this channel. */
		NFL_EVENTBUS_NODISCARD bool AddPublisher(UObject* PublisherObj, const FPublisherBinding& Binding);
		/** @brief Removes a publisher and unbinds listeners from it. */
		NFL_EVENTBUS_NODISCARD bool RemovePublisher(UObject* PublisherObj);

		/** @brief Registers or updates one listener callback for this channel. */
		NFL_EVENTBUS_NODISCARD bool AddListener(UObject* ListenerObj, const FListenerBinding& Binding);
		/** @brief Removes one listener callback (or all callbacks for object in owning mode). */
		NFL_EVENTBUS_NODISCARD bool RemoveListener(UObject* ListenerObj, const FListenerBinding& Binding);

		/** @brief Unbinds every callback and clears publishers/listeners for this channel. */
		void ClearAndUnbind();

	private:
		/** @brief Removes dead publisher entries and refreshes cached channel signature data. */
		void CleanupPublishers();
		/** @brief Removes dead listener entries and detaches stale callbacks from live publishers. */
		void CleanupListeners();
		/** @brief Recomputes channel signature metadata from current live publishers. */
		void RefreshChannelSignature();

		/**
		 * @brief Returns true when listener entry should be treated as stale for cleanup.
		 */
		NFL_EVENTBUS_NODISCARD bool IsListenerEntryStale(const FListenerEntry& ListenerEntry) const;

		/**
		 * @brief Removes callback binding from a publisher delegate.
		 *
		 * This helper always removes the exact callback binding represented by Callback.
		 * Object-wide ownership semantics are handled by remove-path key selection logic.
		 */
		void RemoveBinding(
			UObject* PublisherObj,
			const FMulticastDelegateProperty* DelegateProperty,
			const FScriptDelegate& Callback) const;

		/** @brief Binds a listener callback to a publisher delegate with duplicate-safe behavior. */
		void BindListenerToPublisher(const FListenerEntry& ListenerEntry, FPublisherEntry& PublisherEntry) const;
		/** @brief Unbinds a listener callback from a publisher delegate. */
		void UnbindListenerFromPublisher(const FListenerEntry& ListenerEntry, FPublisherEntry& PublisherEntry) const;
		/** @brief Unbinds every tracked listener callback from one publisher delegate. */
		void UnbindAllListenersFromPublisher(FPublisherEntry& PublisherEntry) const;

	private:
		bool bOwnsPublisherDelegates = false;
		TArray<FPublisherEntry> Publishers;
		TMap<FListenerKey, FListenerEntry> Listeners;
		const UFunction* ChannelDelegateSignature = nullptr;
		FName ChannelDelegatePropertyName = NAME_None;
	};
} // namespace Nfrrlib::EventBus::Private
