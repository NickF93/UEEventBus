#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"

#include "K2Node_EventBusAddListenerValidated.generated.h"

/**
 * @brief Blueprint node with listener-function picker filtered by selected listener class.
 */
UCLASS()
class EVENTBUSEDITOR_API UK2Node_EventBusAddListenerValidated : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	/** @brief Configures node to call `UEventBusBlueprintLibrary::AddListenerValidated`. */
	UK2Node_EventBusAddListenerValidated();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(class FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	/**
	 * @brief Builds dropdown options for FunctionName pin from selected listener class.
	 */
	static void BuildFunctionOptions(const UEdGraphPin* FunctionNamePin, TArray<TSharedPtr<FName>>& OutOptions);

	/** @brief Name of object input pin used to infer listener class for filtering. */
	static const FName ListenerObjPinName;
	/** @brief Name of function pin that is rendered as filtered dropdown. */
	static const FName FunctionNamePinName;
};
