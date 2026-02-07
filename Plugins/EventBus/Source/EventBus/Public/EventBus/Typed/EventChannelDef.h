#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "EventBus/Core/EventBusAttributes.h"

#include <concepts>

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Compile-time contract for typed channel definitions.
	 */
	template <typename TChannelDef>
	concept CEventChannelDef = requires(typename TChannelDef::PublisherType* PublisherPtr)
	{
		typename TChannelDef::PublisherType;
		typename TChannelDef::FDelegate;
		{ TChannelDef::GetChannelTag() } -> std::convertible_to<FGameplayTag>;
		{ TChannelDef::GetDelegatePropertyName() } -> std::convertible_to<FName>;
		{ PublisherPtr->*TChannelDef::DelegateMember } -> std::same_as<typename TChannelDef::FDelegate&>;
	};
} // namespace Nfrrlib::EventBus

/**
 * @brief Declares a typed channel for C++ static API helpers.
 */
#define NFL_DECLARE_EVENTBUS_CHANNEL(ChannelDefName, DelegateType, PublisherClass, ChannelTagExpr, DelegateMemberName) \
	struct ChannelDefName final                                                                                         \
	{                                                                                                                    \
		using FDelegate = DelegateType;                                                                                  \
		using PublisherType = PublisherClass;                                                                            \
		NFL_EVENTBUS_NODISCARD static const FGameplayTag& GetChannelTag()                                              \
		{                                                                                                                \
			static const FGameplayTag Tag = (ChannelTagExpr);                                                            \
			return Tag;                                                                                                   \
		}                                                                                                                \
		NFL_EVENTBUS_NODISCARD static FName GetDelegatePropertyName()                                                   \
		{                                                                                                                \
			return GET_MEMBER_NAME_CHECKED(PublisherType, DelegateMemberName);                                           \
		}                                                                                                                \
		static constexpr FDelegate PublisherType::* DelegateMember = &PublisherType::DelegateMemberName;                \
	}
