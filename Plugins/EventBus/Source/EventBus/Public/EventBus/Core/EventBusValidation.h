#pragma once

#include "CoreMinimal.h"
#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBusErrors.h"
#include "EventBus/Core/EventBusTypes.h"

class FMulticastDelegateProperty;
class UFunction;

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Shared validation helpers for EventBus core operations.
	 *
	 * This utility has no state and keeps validation logic outside orchestration code.
	 */
	class EVENTBUS_API FEventBusValidation final
	{
	public:
		/** @brief Verifies that EventBus API access is happening from the game thread. */
		NFL_EVENTBUS_NODISCARD static bool EnsureGameThread(const TCHAR* Context, EEventBusError& OutError);
		/** @brief Validates a non-empty channel tag. */
		NFL_EVENTBUS_NODISCARD static bool ValidateChannelTag(const FGameplayTag& ChannelTag, EEventBusError& OutError);
		/** @brief Validates UObject lifetime and pointer safety. */
		NFL_EVENTBUS_NODISCARD static bool ValidateObject(UObject* Object, EEventBusError& OutError);
		/** @brief Validates a non-None reflective binding name. */
		NFL_EVENTBUS_NODISCARD static bool ValidateName(FName Name, EEventBusError& OutError);

		/** @brief Resolves and type-checks a multicast delegate property from a publisher object. */
		NFL_EVENTBUS_NODISCARD
		static const FMulticastDelegateProperty* ResolveDelegateProperty(
			UObject* PublisherObj,
			FName DelegatePropertyName,
			EEventBusError& OutError);

		/** @brief Resolves and validates a listener UFunction by name. */
		NFL_EVENTBUS_NODISCARD
		static const UFunction* ResolveListenerFunction(
			UObject* ListenerObj,
			FName FunctionName,
			EEventBusError& OutError);

		/** @brief Checks if a listener function signature matches a delegate signature. */
		NFL_EVENTBUS_NODISCARD
		static bool IsFunctionCompatibleWithDelegate(
			const UFunction* ListenerFunction,
			const FMulticastDelegateProperty* DelegateProperty,
			EEventBusError& OutError);

		/** @brief Builds an EventBus listener delegate binding and returns resolved function metadata. */
		NFL_EVENTBUS_NODISCARD
		static bool BuildListenerBinding(
			UObject* ListenerObj,
			FName FunctionName,
			const UFunction*& OutListenerFunction,
			FScriptDelegate& OutDelegate,
			EEventBusError& OutError);
	};
} // namespace Nfrrlib::EventBus
