/**
 * @file EventTags.h
 * @brief Macros for defining EventBus tag types.
 */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * @def NFL_DECLARE_EVENT_TAG
 * @brief Declares an EventTag backed by an FGameplayTag publisher id.
 *
 * The generated tag type exposes:
 * - `using FDelegate`
 * - `using PublisherType`
 * - `static FGameplayTag GetPublisherId()`
 * - `static constexpr FDelegate PublisherType::* DelegateMember`
 */
#ifndef NFL_DECLARE_EVENT_TAG
#define NFL_DECLARE_EVENT_TAG(TagTypeName, DelegateType, PublisherClass, PublisherTagExpr, DelegateMemberName)       \
	struct TagTypeName final                                                                                          \
	{                                                                                                                  \
		/* Delegate type for this tag */                                                                              \
		using FDelegate = DelegateType;                                                                               \
                                                                                                                       \
		/* Publisher class that owns the multicast delegate */                                                        \
		using PublisherType = PublisherClass;                                                                         \
                                                                                                                       \
		/* Publisher channel id */                                                                                    \
		static const FGameplayTag& GetPublisherId()                                                                   \
		{                                                                                                             \
			static const FGameplayTag Tag = (PublisherTagExpr);                                                       \
			return Tag;                                                                                                \
		}                                                                                                             \
                                                                                                                       \
		/* Pointer-to-member to reach the delegate on the publisher */                                                \
		static constexpr FDelegate PublisherType::* DelegateMember = &PublisherType::DelegateMemberName;             \
	};
#endif
