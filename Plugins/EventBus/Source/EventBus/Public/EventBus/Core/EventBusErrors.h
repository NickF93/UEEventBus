#pragma once

#include "CoreMinimal.h"
#include "EventBus/Core/EventBusAttributes.h"

namespace Nfrrlib::EventBus
{
	/**
	 * @brief High-level validation/operation outcomes used for diagnostics.
	 */
	enum class EEventBusError : uint8
	{
		None = 0,
		NotGameThread,
		InvalidChannel,
		ChannelNotRegistered,
		InvalidObject,
		InvalidBindingName,
		DelegatePropertyNotFound,
		ListenerFunctionNotBindable,
		SignatureMismatch,
		OwnershipPolicyConflict
	};

	/**
	 * @brief Returns a readable string for EEventBusError values.
	 */
	NFL_EVENTBUS_NODISCARD EVENTBUS_API const TCHAR* LexToString(EEventBusError Error);
} // namespace Nfrrlib::EventBus
