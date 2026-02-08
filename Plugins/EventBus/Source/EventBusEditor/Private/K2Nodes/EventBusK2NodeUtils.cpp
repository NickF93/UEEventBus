#include "K2Nodes/EventBusK2NodeUtils.h"

#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node.h"
#include "UObject/Class.h"

namespace EventBusK2NodeUtils
{
	namespace
	{
		/**
		 * @brief Resolves a concrete class from K2 pin type metadata when available.
		 */
		UClass* ResolveClassFromPinType(const FEdGraphPinType& PinType)
		{
			return Cast<UClass>(PinType.PinSubCategoryObject.Get());
		}

		/**
		 * @brief Returns true when class metadata is only generic UObject and not useful for filtering.
		 */
		bool IsGenericObjectClass(const UClass* Class)
		{
			return Class == UObject::StaticClass();
		}

		/**
		 * @brief Resolves owning Blueprint generated/skeleton class as final fallback for self-context nodes.
		 */
		UClass* ResolveOwningBlueprintClass(const UEdGraphPin* ObjectPin)
		{
			if (!ObjectPin)
			{
				return nullptr;
			}

			const UK2Node* const K2Node = Cast<UK2Node>(ObjectPin->GetOwningNode());
			if (!K2Node)
			{
				return nullptr;
			}

			const UBlueprint* const Blueprint = K2Node->GetBlueprint();
			if (!Blueprint)
			{
				return nullptr;
			}

			if (UClass* const GeneratedClass = Blueprint->GeneratedClass)
			{
				return GeneratedClass;
			}

			return Blueprint->SkeletonGeneratedClass;
		}
	}

	/**
	 * @brief Resolves the most specific object class for picker filtering from defaults, links, pin type, or owning Blueprint.
	 */
	UClass* ResolveObjectClassFromPin(const UEdGraphPin* ObjectPin)
	{
		if (!ObjectPin)
		{
			return nullptr;
		}

		if (ObjectPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Object &&
			ObjectPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Interface)
		{
			return nullptr;
		}

		if (::IsValid(ObjectPin->DefaultObject))
		{
			return ObjectPin->DefaultObject->GetClass();
		}

		UClass* FallbackClass = nullptr;
		for (const UEdGraphPin* const LinkedPin : ObjectPin->LinkedTo)
		{
			if (::IsValid(LinkedPin->DefaultObject))
			{
				return LinkedPin->DefaultObject->GetClass();
			}

			if (UClass* const LinkedClass = ResolveClassFromPinType(LinkedPin->PinType))
			{
				if (!IsGenericObjectClass(LinkedClass))
				{
					return LinkedClass;
				}

				FallbackClass = LinkedClass;
			}
		}

		if (UClass* const PinClass = ResolveClassFromPinType(ObjectPin->PinType))
		{
			if (!IsGenericObjectClass(PinClass))
			{
				return PinClass;
			}

			if (!FallbackClass)
			{
				FallbackClass = PinClass;
			}
		}

		if (UClass* const BlueprintClass = ResolveOwningBlueprintClass(ObjectPin))
		{
			return BlueprintClass;
		}

		return FallbackClass;
	}
}
