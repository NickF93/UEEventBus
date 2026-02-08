#include "K2Nodes/K2Node_EventBusAddListenerValidated.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraphPin.h"
#include "EventBus/BP/EventBusBlueprintLibrary.h"
#include "K2Nodes/EventBusK2NodeUtils.h"
#include "UObject/FieldIterator.h"

#define LOCTEXT_NAMESPACE "EventBusK2NodeAddListenerValidated"

const FName UK2Node_EventBusAddListenerValidated::ListenerObjPinName(TEXT("ListenerObj"));
const FName UK2Node_EventBusAddListenerValidated::FunctionNamePinName(TEXT("FunctionName"));

/**
 * @brief Binds this custom node to the runtime BP API entry point.
 */
UK2Node_EventBusAddListenerValidated::UK2Node_EventBusAddListenerValidated()
{
	FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UEventBusBlueprintLibrary, AddListenerValidated),
		UEventBusBlueprintLibrary::StaticClass());
}

/**
 * @brief Returns display title shown in Blueprint graph.
 */
FText UK2Node_EventBusAddListenerValidated::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Add Listener Validated (Filtered)");
}

/**
 * @brief Returns tooltip text shown in Blueprint graph.
 */
FText UK2Node_EventBusAddListenerValidated::GetTooltipText() const
{
	return LOCTEXT("Tooltip", "Adds a validated EventBus listener. FunctionName dropdown is filtered from the selected listener class.");
}

/**
 * @brief Returns palette menu category for this node.
 */
FText UK2Node_EventBusAddListenerValidated::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "EventBus|Validated");
}

/**
 * @brief Registers this custom node in Blueprint action database.
 */
void UK2Node_EventBusAddListenerValidated::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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
 * @brief Builds list picker entries from Blueprint-callable functions declared on selected listener class.
 */
void UK2Node_EventBusAddListenerValidated::BuildFunctionOptions(
	const UEdGraphPin* FunctionNamePin,
	TArray<TSharedPtr<FName>>& OutOptions)
{
	OutOptions.Empty();
	if (!FunctionNamePin)
	{
		return;
	}

	const UK2Node_EventBusAddListenerValidated* const OwningNode =
		Cast<UK2Node_EventBusAddListenerValidated>(FunctionNamePin->GetOwningNode());
	if (!OwningNode)
	{
		return;
	}

	const UEdGraphPin* const ListenerPin = OwningNode->FindPin(ListenerObjPinName);
	UClass* const ListenerClass = EventBusK2NodeUtils::ResolveObjectClassFromPin(ListenerPin);
	if (!::IsValid(ListenerClass))
	{
		return;
	}

	TSet<FName> UniqueNames;
	for (TFieldIterator<UFunction> It(ListenerClass, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		const UFunction* const Function = *It;
		if (!Function)
		{
			continue;
		}

		if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent))
		{
			continue;
		}

		if (Function->HasAnyFunctionFlags(FUNC_Delegate))
		{
			continue;
		}

		const FName FunctionName = Function->GetFName();
		if (FunctionName.IsNone() || FunctionName.ToString().StartsWith(TEXT("ExecuteUbergraph_")))
		{
			continue;
		}

		UniqueNames.Add(FunctionName);
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
