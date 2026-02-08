#include "K2Nodes/K2Node_EventBusAddPublisherValidated.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraphPin.h"
#include "EventBus/BP/EventBusBlueprintLibrary.h"
#include "K2Nodes/EventBusK2NodeUtils.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "EventBusK2NodeAddPublisherValidated"

const FName UK2Node_EventBusAddPublisherValidated::PublisherObjPinName(TEXT("PublisherObj"));
const FName UK2Node_EventBusAddPublisherValidated::DelegatePropertyPinName(TEXT("DelegatePropertyName"));

/**
 * @brief Binds this custom node to the runtime BP API entry point.
 */
UK2Node_EventBusAddPublisherValidated::UK2Node_EventBusAddPublisherValidated()
{
	FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UEventBusBlueprintLibrary, AddPublisherValidated),
		UEventBusBlueprintLibrary::StaticClass());
}

/**
 * @brief Returns display title shown in Blueprint graph.
 */
FText UK2Node_EventBusAddPublisherValidated::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Add Publisher Validated (Filtered)");
}

/**
 * @brief Returns tooltip text shown in Blueprint graph.
 */
FText UK2Node_EventBusAddPublisherValidated::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Adds a validated EventBus publisher. Delegate dropdown is filtered from the selected publisher class.");
}

/**
 * @brief Returns palette menu category for this node.
 */
FText UK2Node_EventBusAddPublisherValidated::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "EventBus|Validated");
}

/**
 * @brief Registers this custom node in Blueprint action database.
 */
void UK2Node_EventBusAddPublisherValidated::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	const UClass* const ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* const NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner);
		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

/**
 * @brief Builds list picker entries from delegate properties declared on selected publisher class.
 */
void UK2Node_EventBusAddPublisherValidated::BuildDelegateOptions(
	const UEdGraphPin* DelegatePin,
	TArray<TSharedPtr<FName>>& OutOptions)
{
	OutOptions.Empty();
	if (!DelegatePin)
	{
		return;
	}

	const UK2Node_EventBusAddPublisherValidated* const OwningNode =
		Cast<UK2Node_EventBusAddPublisherValidated>(DelegatePin->GetOwningNode());
	if (!OwningNode)
	{
		return;
	}

	const UEdGraphPin* const PublisherPin = OwningNode->FindPin(PublisherObjPinName);
	UClass* const PublisherClass = EventBusK2NodeUtils::ResolveObjectClassFromPin(PublisherPin);
	if (!::IsValid(PublisherClass))
	{
		return;
	}

	TSet<FName> UniqueNames;
	for (TFieldIterator<FMulticastDelegateProperty> It(PublisherClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const FMulticastDelegateProperty* const DelegateProperty = *It;
		if (!DelegateProperty)
		{
			continue;
		}

		const FName PropertyName = DelegateProperty->GetFName();
		if (!PropertyName.IsNone())
		{
			UniqueNames.Add(PropertyName);
		}
	}

	TArray<FName> SortedNames = UniqueNames.Array();
	SortedNames.Sort([](const FName& A, const FName& B)
	{
		return A.Compare(B) < 0;
	});

	for (const FName Name : SortedNames)
	{
		OutOptions.Add(MakeShared<FName>(Name));
	}
}

#undef LOCTEXT_NAMESPACE
