#pragma once

#include "CoreMinimal.h"

/**
 * @brief EventBus-local attribute aliases for consistent C++20 annotation usage.
 *
 * These aliases intentionally avoid hard dependency on compiler-specific macros and
 * degrade safely when a compiler does not expose the target C++ attribute.
 */

#if !defined(NFL_EVENTBUS_MAYBE_UNUSED)
	#if defined(__has_cpp_attribute)
		#if __has_cpp_attribute(maybe_unused)
			#define NFL_EVENTBUS_MAYBE_UNUSED [[maybe_unused]]
		#else
			#define NFL_EVENTBUS_MAYBE_UNUSED
		#endif
	#else
		#define NFL_EVENTBUS_MAYBE_UNUSED
	#endif
#endif

#if !defined(NFL_EVENTBUS_NODISCARD)
	#if defined(__has_cpp_attribute)
		#if __has_cpp_attribute(nodiscard)
			#define NFL_EVENTBUS_NODISCARD [[nodiscard]]
		#else
			#define NFL_EVENTBUS_NODISCARD
		#endif
	#else
		#define NFL_EVENTBUS_NODISCARD
	#endif
#endif

#if !defined(NFL_EVENTBUS_NODISCARD_MSG)
	#if defined(__has_cpp_attribute)
		#if __has_cpp_attribute(nodiscard)
			#define NFL_EVENTBUS_NODISCARD_MSG(MessageLiteral) [[nodiscard(MessageLiteral)]]
		#else
			#define NFL_EVENTBUS_NODISCARD_MSG(MessageLiteral)
		#endif
	#else
		#define NFL_EVENTBUS_NODISCARD_MSG(MessageLiteral)
	#endif
#endif

#if !defined(NFL_EVENTBUS_UNUSED)
	#define NFL_EVENTBUS_UNUSED(Value) static_cast<void>(Value)
#endif

