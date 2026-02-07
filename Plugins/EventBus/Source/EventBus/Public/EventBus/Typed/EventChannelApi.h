#pragma once

#include "CoreMinimal.h"
#include "Templates/IsPointer.h"
#include "Templates/UnrealTypeTraits.h"
#include "Traits/MemberFunctionPtrOuter.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBus.h"
#include "EventBus/Typed/EventChannelDef.h"

#include <type_traits>

namespace Nfrrlib::EventBus
{
	namespace Detail
	{
		template <typename T>
		using TNoCVRef = std::remove_cv_t<typename TRemoveReference<T>::Type>;

		template <typename TObject>
		using TObjectNoRef = typename TRemoveReference<TObject>::Type;

		template <typename TObject>
		using TObjectNoCV = std::remove_cv_t<TObjectNoRef<TObject>>;

		template <typename TObject>
		using TObjectNoPtr = typename TRemovePointer<TObjectNoCV<TObject>>::Type;

		template <typename T>
		concept CMemberFunctionPointer = requires { typename TMemberFunctionPtrOuter_T<TNoCVRef<T>>; };

		/**
		 * @brief Performs compile-time listener object/function contract checks and returns reflective function name.
		 */
		template <typename TObject, typename TFunc>
		FORCEINLINE FName ValidateListenerFunc(const FName FunctionName)
		{
			static_assert(TIsPointer<TObjectNoRef<TObject>>::Value, "Listener object must be a UObject pointer.");
			static_assert(TIsDerivedFrom<TObjectNoPtr<TObject>, UObject>::Value, "Listener object must derive from UObject.");
			static_assert(CMemberFunctionPointer<TFunc>, "Listener function must be a member-function pointer.");

			if constexpr (CMemberFunctionPointer<TFunc>)
			{
				using FuncClass = TMemberFunctionPtrOuter_T<TNoCVRef<TFunc>>;
				static_assert(TIsDerivedFrom<TObjectNoPtr<TObject>, FuncClass>::Value,
					"Listener function pointer class must match listener object type.");
			}

			return FunctionName;
		}
	} // namespace Detail

	/**
	 * @brief Captures a member-function pointer plus its static FName.
	 */
	template <typename TFunc>
	struct TEventListenerMethod final
	{
		/** @brief Raw member-function pointer used for compile-time class validation. */
		NFL_EVENTBUS_MAYBE_UNUSED TFunc FunctionPtr{};
		/** @brief Reflected function name used by runtime binding. */
		FName FunctionName = NAME_None;
	};

	/**
	 * @brief Creates a typed listener method wrapper.
	 */
	template <typename TFunc>
	NFL_EVENTBUS_NODISCARD FORCEINLINE TEventListenerMethod<TFunc> MakeEventListenerMethod(TFunc FunctionPtr, FName FunctionName)
	{
		TEventListenerMethod<TFunc> Method;
		Method.FunctionPtr = FunctionPtr;
		Method.FunctionName = FunctionName;
		return Method;
	}

	/**
	 * @brief Static typed API for one channel definition.
	 */
	template <CEventChannelDef TChannelDef>
	class TEventChannelApi final
	{
	public:
		/** @brief Registers this typed channel in the runtime bus. */
		NFL_EVENTBUS_NODISCARD static bool Register(FEventBus& Bus, const bool bOwnsPublisherDelegates = false)
		{
			FChannelRegistration Registration;
			Registration.ChannelTag = TChannelDef::GetChannelTag();
			Registration.bOwnsPublisherDelegates = bOwnsPublisherDelegates;
			return Bus.RegisterChannel(Registration);
		}

		/** @brief Binds a publisher instance using compile-time delegate property metadata. */
		NFL_EVENTBUS_NODISCARD static bool AddPublisher(FEventBus& Bus, typename TChannelDef::PublisherType* PublisherObj)
		{
			FPublisherBinding Binding;
			Binding.DelegatePropertyName = TChannelDef::GetDelegatePropertyName();
			return Bus.AddPublisher(TChannelDef::GetChannelTag(), PublisherObj, Binding);
		}

		/** @brief Removes a publisher instance from this typed channel. */
		NFL_EVENTBUS_NODISCARD static bool RemovePublisher(FEventBus& Bus, typename TChannelDef::PublisherType* PublisherObj)
		{
			return Bus.RemovePublisher(TChannelDef::GetChannelTag(), PublisherObj);
		}

		template <typename TListener, typename TFunc>
		/** @brief Adds a listener callback to this typed channel. */
		NFL_EVENTBUS_NODISCARD static bool AddListener(FEventBus& Bus, TListener* ListenerObj, const TEventListenerMethod<TFunc>& Method)
		{
			FListenerBinding Binding;
			Binding.FunctionName = Detail::ValidateListenerFunc<decltype(ListenerObj), TFunc>(Method.FunctionName);
			return Bus.AddListener(TChannelDef::GetChannelTag(), ListenerObj, Binding);
		}

		template <typename TListener, typename TFunc>
		/** @brief Removes a listener callback from this typed channel. */
		NFL_EVENTBUS_NODISCARD static bool RemoveListener(FEventBus& Bus, TListener* ListenerObj, const TEventListenerMethod<TFunc>& Method)
		{
			FListenerBinding Binding;
			Binding.FunctionName = Detail::ValidateListenerFunc<decltype(ListenerObj), TFunc>(Method.FunctionName);
			return Bus.RemoveListener(TChannelDef::GetChannelTag(), ListenerObj, Binding);
		}
	};
} // namespace Nfrrlib::EventBus

/**
 * @brief Convenience helper for pointer-based listener methods.
 */
#define NFL_EVENTBUS_METHOD(ClassType, FunctionName) \
	::Nfrrlib::EventBus::MakeEventListenerMethod(&ClassType::FunctionName, GET_FUNCTION_NAME_CHECKED(ClassType, FunctionName))

/**
 * @brief Adds a listener on a typed channel using pointer syntax.
 */
#define NFL_EVENTBUS_ADD_LISTENER(Bus, ChannelDef, ListenerObj, ClassType, FunctionName) \
	::Nfrrlib::EventBus::TEventChannelApi<ChannelDef>::AddListener((Bus), (ListenerObj), NFL_EVENTBUS_METHOD(ClassType, FunctionName))

/**
 * @brief Removes a listener on a typed channel using pointer syntax.
 */
#define NFL_EVENTBUS_REMOVE_LISTENER(Bus, ChannelDef, ListenerObj, ClassType, FunctionName) \
	::Nfrrlib::EventBus::TEventChannelApi<ChannelDef>::RemoveListener((Bus), (ListenerObj), NFL_EVENTBUS_METHOD(ClassType, FunctionName))
