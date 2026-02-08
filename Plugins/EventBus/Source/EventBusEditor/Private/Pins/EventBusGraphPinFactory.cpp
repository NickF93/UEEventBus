#include "Pins/EventBusGraphPinFactory.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Nodes/K2Node_EventBusAddListenerValidated.h"
#include "K2Nodes/K2Node_EventBusAddPublisherValidated.h"
#include "SGraphPinNameList.h"

/**
 * @brief Creates name-list pin widgets for EventBus filtered custom nodes.
 *
 * This factory only customizes `FName` pins that map to listener function or
 * publisher delegate member names on EventBus filtered K2 nodes.
 */
TSharedPtr<SGraphPin> FEventBusGraphPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (!InPin || InPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Name)
	{
		return nullptr;
	}

	const UEdGraphNode* const OwningNode = InPin->GetOwningNode();
	if (!OwningNode)
	{
		return nullptr;
	}

	TArray<TSharedPtr<FName>> NameOptions;

	if (Cast<UK2Node_EventBusAddListenerValidated>(OwningNode) &&
		InPin->PinName == UK2Node_EventBusAddListenerValidated::FunctionNamePinName)
	{
		UK2Node_EventBusAddListenerValidated::BuildFunctionOptions(InPin, NameOptions);
		return SNew(SGraphPinNameList, InPin, NameOptions);
	}

	if (Cast<UK2Node_EventBusAddPublisherValidated>(OwningNode) &&
		InPin->PinName == UK2Node_EventBusAddPublisherValidated::DelegatePropertyPinName)
	{
		UK2Node_EventBusAddPublisherValidated::BuildDelegateOptions(InPin, NameOptions);
		return SNew(SGraphPinNameList, InPin, NameOptions);
	}

	return nullptr;
}
