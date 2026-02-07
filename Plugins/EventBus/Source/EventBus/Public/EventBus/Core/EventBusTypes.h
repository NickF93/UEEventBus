#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "UObject/ObjectKey.h"
#include "UObject/ScriptDelegates.h"
#include "UObject/WeakObjectPtr.h"

class FMulticastDelegateProperty;

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Channel registration policy.
	 */
	struct FChannelRegistration final
	{
		/** @brief Logical routing channel key. Must be valid. */
		FGameplayTag ChannelTag;
		/** @brief Removes all EventBus-managed callbacks for a listener object when true. */
		bool bOwnsPublisherDelegates = false;
	};

	/**
	 * @brief Runtime publisher binding descriptor.
	 */
	struct FPublisherBinding final
	{
		/** @brief Reflected multicast delegate property name on publisher class. */
		FName DelegatePropertyName = NAME_None;
	};

	/**
	 * @brief Runtime listener binding descriptor.
	 */
	struct FListenerBinding final
	{
		/** @brief Reflected listener UFUNCTION name on listener class. */
		FName FunctionName = NAME_None;
	};

	/**
	 * @brief Stable listener identity key (object identity + function).
	 */
	struct FListenerKey final
	{
		/** @brief Stable object identity independent from UObject rename operations. */
		FObjectKey ListenerObjectKey;
		/** @brief Listener function name bound for this object. */
		FName FunctionName = NAME_None;

		friend bool operator==(const FListenerKey& Lhs, const FListenerKey& Rhs)
		{
			return Lhs.ListenerObjectKey == Rhs.ListenerObjectKey && Lhs.FunctionName == Rhs.FunctionName;
		}

		friend uint32 GetTypeHash(const FListenerKey& Key)
		{
			return HashCombine(GetTypeHash(Key.ListenerObjectKey), GetTypeHash(Key.FunctionName));
		}
	};

	/**
	 * @brief Internal publisher storage for one channel.
	 */
	struct FPublisherEntry final
	{
		/** @brief Weak publisher reference for stale-object cleanup safety. */
		TWeakObjectPtr<UObject> Publisher;
		/** @brief Delegate property name used during registration. */
		FName DelegatePropertyName = NAME_None;
		/** @brief Cached delegate property for fast bind/unbind. */
		const FMulticastDelegateProperty* DelegateProperty = nullptr;
	};

	/**
	 * @brief Internal listener storage for one channel.
	 */
	struct FListenerEntry final
	{
		/** @brief Stable key used for deduplication and remove operations. */
		FListenerKey ListenerKey;
		/** @brief Weak listener reference for stale-object cleanup safety. */
		TWeakObjectPtr<UObject> Listener;
		/** @brief Function name used during registration. */
		FName FunctionName = NAME_None;
		/** @brief Cached function pointer used for signature validation. */
		const UFunction* ListenerFunction = nullptr;
		/** @brief Script delegate callback bound to publisher multicast delegates. */
		FScriptDelegate Callback;
	};
} // namespace Nfrrlib::EventBus
