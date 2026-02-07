#include "EventBus/Core/EventBusValidation.h"

#include "UObject/UnrealType.h"

#include "EventBus/Core/EventBus.h"

namespace Nfrrlib::EventBus
{
	/**
	 * @brief Converts EventBus error enum values into stable diagnostic strings.
	 */
	const TCHAR* LexToString(const EEventBusError Error)
	{
		switch (Error)
		{
		case EEventBusError::None:
			return TEXT("None");
		case EEventBusError::NotGameThread:
			return TEXT("NotGameThread");
		case EEventBusError::InvalidChannel:
			return TEXT("InvalidChannel");
		case EEventBusError::ChannelNotRegistered:
			return TEXT("ChannelNotRegistered");
		case EEventBusError::InvalidObject:
			return TEXT("InvalidObject");
		case EEventBusError::InvalidBindingName:
			return TEXT("InvalidBindingName");
		case EEventBusError::DelegatePropertyNotFound:
			return TEXT("DelegatePropertyNotFound");
		case EEventBusError::ListenerFunctionNotBindable:
			return TEXT("ListenerFunctionNotBindable");
		case EEventBusError::SignatureMismatch:
			return TEXT("SignatureMismatch");
		case EEventBusError::OwnershipPolicyConflict:
			return TEXT("OwnershipPolicyConflict");
		default:
			return TEXT("UnknownError");
		}
	}

	/**
	 * @brief Guard helper that enforces game-thread-only EventBus usage.
	 */
	bool FEventBusValidation::EnsureGameThread(const TCHAR* Context, EEventBusError& OutError)
	{
		if (IsInGameThread())
		{
			OutError = EEventBusError::None;
			return true;
		}

		OutError = EEventBusError::NotGameThread;
		UE_LOG(LogNFLEventBus, Warning, TEXT("EventBus: %s must be called on the Game Thread."), Context);
		return false;
	}

	/**
	 * @brief Validates that routing channel key is initialized.
	 */
	bool FEventBusValidation::ValidateChannelTag(const FGameplayTag& ChannelTag, EEventBusError& OutError)
	{
		if (ChannelTag.IsValid())
		{
			OutError = EEventBusError::None;
			return true;
		}

		OutError = EEventBusError::InvalidChannel;
		return false;
	}

	/**
	 * @brief Validates pointer and UObject lifetime state.
	 */
	bool FEventBusValidation::ValidateObject(UObject* Object, EEventBusError& OutError)
	{
		if (::IsValid(Object))
		{
			OutError = EEventBusError::None;
			return true;
		}

		OutError = EEventBusError::InvalidObject;
		return false;
	}

	/**
	 * @brief Validates reflective bind names (function/property).
	 */
	bool FEventBusValidation::ValidateName(const FName Name, EEventBusError& OutError)
	{
		if (!Name.IsNone())
		{
			OutError = EEventBusError::None;
			return true;
		}

		OutError = EEventBusError::InvalidBindingName;
		return false;
	}

	/**
	 * @brief Resolves and validates a multicast delegate property from a publisher object.
	 */
	const FMulticastDelegateProperty* FEventBusValidation::ResolveDelegateProperty(
		UObject* PublisherObj,
		const FName DelegatePropertyName,
		EEventBusError& OutError)
	{
		if (!ValidateObject(PublisherObj, OutError) || !ValidateName(DelegatePropertyName, OutError))
		{
			return nullptr;
		}

		const FProperty* Property = PublisherObj->GetClass()->FindPropertyByName(DelegatePropertyName);
		const FMulticastDelegateProperty* DelegateProperty = CastField<const FMulticastDelegateProperty>(Property);
		if (DelegateProperty == nullptr)
		{
			OutError = EEventBusError::DelegatePropertyNotFound;
			return nullptr;
		}

		OutError = EEventBusError::None;
		return DelegateProperty;
	}

	/**
	 * @brief Resolves and validates listener function metadata from function name.
	 */
	const UFunction* FEventBusValidation::ResolveListenerFunction(
		UObject* ListenerObj,
		const FName FunctionName,
		EEventBusError& OutError)
	{
		if (!ValidateObject(ListenerObj, OutError) || !ValidateName(FunctionName, OutError))
		{
			return nullptr;
		}

		const UFunction* ListenerFunction = ListenerObj->FindFunction(FunctionName);
		if (ListenerFunction == nullptr)
		{
			OutError = EEventBusError::ListenerFunctionNotBindable;
			return nullptr;
		}

		OutError = EEventBusError::None;
		return ListenerFunction;
	}

	/**
	 * @brief Checks bidirectional signature compatibility between listener function and delegate signature.
	 */
	bool FEventBusValidation::IsFunctionCompatibleWithDelegate(
		const UFunction* ListenerFunction,
		const FMulticastDelegateProperty* DelegateProperty,
		EEventBusError& OutError)
	{
		if (ListenerFunction == nullptr)
		{
			OutError = EEventBusError::ListenerFunctionNotBindable;
			return false;
		}

		if (DelegateProperty == nullptr || DelegateProperty->SignatureFunction == nullptr)
		{
			OutError = EEventBusError::DelegatePropertyNotFound;
			return false;
		}

		const UFunction* Signature = DelegateProperty->SignatureFunction;
		const bool bCompatible =
			ListenerFunction->IsSignatureCompatibleWith(Signature) &&
			Signature->IsSignatureCompatibleWith(ListenerFunction);

		OutError = bCompatible ? EEventBusError::None : EEventBusError::SignatureMismatch;
		return bCompatible;
	}

	/**
	 * @brief Builds a script delegate callback and returns resolved listener function metadata.
	 */
	bool FEventBusValidation::BuildListenerBinding(
		UObject* ListenerObj,
		const FName FunctionName,
		const UFunction*& OutListenerFunction,
		FScriptDelegate& OutDelegate,
		EEventBusError& OutError)
	{
		const UFunction* ListenerFunction = ResolveListenerFunction(ListenerObj, FunctionName, OutError);
		if (ListenerFunction == nullptr)
		{
			return false;
		}

		OutDelegate = FScriptDelegate();
		OutDelegate.BindUFunction(ListenerObj, FunctionName);
		if (!OutDelegate.IsBound())
		{
			OutError = EEventBusError::ListenerFunctionNotBindable;
			return false;
		}

		OutListenerFunction = ListenerFunction;
		OutError = EEventBusError::None;
		return true;
	}
} // namespace Nfrrlib::EventBus
