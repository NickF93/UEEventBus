#include "EventBus/Core/EventBus.h"

#include "EventBus/Core/EventBusValidation.h"
#include "Core/EventChannelState.h"

DEFINE_LOG_CATEGORY(LogNFLEventBus);

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Deletes opaque channel state allocated by core runtime.
	 */
	void Private::FEventChannelStateDeleter::operator()(Private::FEventChannelState* const ChannelState) const
	{
		delete ChannelState;
	}

	/**
	 * @brief Destructor ensures deterministic teardown of all channel bindings.
	 */
	FEventBus::~FEventBus()
	{
		Reset();
	}

	/**
	 * @brief Registers a channel with ownership policy; idempotent for same policy.
	 */
	bool FEventBus::RegisterChannel(const FChannelRegistration& Registration)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("RegisterChannel"), Error) ||
			!FEventBusValidation::ValidateChannelTag(Registration.ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RegisterChannel failed. Error=%s Channel=%s"),
				LexToString(Error),
				*Registration.ChannelTag.ToString());
			return false;
		}

		if (const Private::FEventChannelState* Existing = FindChannelState(Registration.ChannelTag))
		{
			if (!Existing->MatchesOwnershipPolicy(Registration.bOwnsPublisherDelegates))
			{
				UE_LOG(LogNFLEventBus, Warning,
					TEXT("RegisterChannel failed. Error=%s Channel=%s ExistingOwns=%d RequestedOwns=%d"),
					LexToString(EEventBusError::OwnershipPolicyConflict),
					*Registration.ChannelTag.ToString(),
					Existing->OwnsPublisherDelegates(),
					Registration.bOwnsPublisherDelegates);
				return false;
			}
			return true;
		}

		Channels.Add(Registration.ChannelTag, FChannelStatePtr(new Private::FEventChannelState(Registration.bOwnsPublisherDelegates)));
		return true;
	}

	/**
	 * @brief Unregisters one channel and unbinds all publisher/listener callbacks.
	 */
	bool FEventBus::UnregisterChannel(const FGameplayTag& ChannelTag)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("UnregisterChannel"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("UnregisterChannel failed. Error=%s Channel=%s"),
				LexToString(Error),
				*ChannelTag.ToString());
			return false;
		}

		Private::FEventChannelState* State = FindChannelState(ChannelTag);
		if (!State)
		{
			return false;
		}

		State->ClearAndUnbind();
		Channels.Remove(ChannelTag);
		return true;
	}

	/**
	 * @brief Returns true when a channel exists in runtime state map.
	 */
	bool FEventBus::IsChannelRegistered(const FGameplayTag& ChannelTag) const
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("IsChannelRegistered"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			return false;
		}

		return FindChannelState(ChannelTag) != nullptr;
	}

	/**
	 * @brief Adds or updates a publisher binding for one registered channel.
	 */
	bool FEventBus::AddPublisher(const FGameplayTag& ChannelTag, UObject* PublisherObj, const FPublisherBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("AddPublisher"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisher failed. Error=%s Channel=%s Publisher=%s"),
				LexToString(Error),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		Private::FEventChannelState* State = FindChannelState(ChannelTag);
		if (!State)
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddPublisher failed. Error=%s Channel=%s"),
				LexToString(EEventBusError::ChannelNotRegistered),
				*ChannelTag.ToString());
			return false;
		}

		return State->AddPublisher(PublisherObj, Binding);
	}

	/**
	 * @brief Removes a publisher binding from one registered channel.
	 */
	bool FEventBus::RemovePublisher(const FGameplayTag& ChannelTag, UObject* PublisherObj)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("RemovePublisher"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemovePublisher failed. Error=%s Channel=%s Publisher=%s"),
				LexToString(Error),
				*ChannelTag.ToString(),
				*GetNameSafe(PublisherObj));
			return false;
		}

		Private::FEventChannelState* State = FindChannelState(ChannelTag);
		if (!State)
		{
			return false;
		}

		return State->RemovePublisher(PublisherObj);
	}

	/**
	 * @brief Adds or updates a listener callback binding for one registered channel.
	 */
	bool FEventBus::AddListener(const FGameplayTag& ChannelTag, UObject* ListenerObj, const FListenerBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("AddListener"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListener failed. Error=%s Channel=%s Listener=%s Func=%s"),
				LexToString(Error),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*Binding.FunctionName.ToString());
			return false;
		}

		Private::FEventChannelState* State = FindChannelState(ChannelTag);
		if (!State)
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("AddListener failed. Error=%s Channel=%s"),
				LexToString(EEventBusError::ChannelNotRegistered),
				*ChannelTag.ToString());
			return false;
		}

		return State->AddListener(ListenerObj, Binding);
	}

	/**
	 * @brief Removes a listener callback binding from one registered channel.
	 */
	bool FEventBus::RemoveListener(const FGameplayTag& ChannelTag, UObject* ListenerObj, const FListenerBinding& Binding)
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("RemoveListener"), Error) ||
			!FEventBusValidation::ValidateChannelTag(ChannelTag, Error))
		{
			UE_LOG(LogNFLEventBus, Warning, TEXT("RemoveListener failed. Error=%s Channel=%s Listener=%s Func=%s"),
				LexToString(Error),
				*ChannelTag.ToString(),
				*GetNameSafe(ListenerObj),
				*Binding.FunctionName.ToString());
			return false;
		}

		Private::FEventChannelState* State = FindChannelState(ChannelTag);
		if (!State)
		{
			return false;
		}

		return State->RemoveListener(ListenerObj, Binding);
	}

	/**
	 * @brief Fully unbinds and clears every registered channel state.
	 */
	void FEventBus::Reset()
	{
		EEventBusError Error = EEventBusError::None;
		if (!FEventBusValidation::EnsureGameThread(TEXT("Reset"), Error))
		{
			return;
		}

		for (TPair<FGameplayTag, FChannelStatePtr>& Pair : Channels)
		{
			if (Pair.Value)
			{
				Pair.Value->ClearAndUnbind();
			}
		}

		Channels.Reset();
	}

	/**
	 * @brief Finds mutable channel state entry.
	 */
	Private::FEventChannelState* FEventBus::FindChannelState(const FGameplayTag& ChannelTag)
	{
		FChannelStatePtr* Found = Channels.Find(ChannelTag);
		return Found ? Found->Get() : nullptr;
	}

	/**
	 * @brief Finds immutable channel state entry.
	 */
	const Private::FEventChannelState* FEventBus::FindChannelState(const FGameplayTag& ChannelTag) const
	{
		const FChannelStatePtr* Found = Channels.Find(ChannelTag);
		return Found ? Found->Get() : nullptr;
	}
} // namespace Nfrrlib::EventBus
