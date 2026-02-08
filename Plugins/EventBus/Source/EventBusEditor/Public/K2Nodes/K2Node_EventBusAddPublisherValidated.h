#pragma once

#include "CoreMinimal.h"
#include "K2Node_CallFunction.h"

#include "K2Node_EventBusAddPublisherValidated.generated.h"

/**
 * @brief Blueprint node with publisher-delegate picker filtered by selected publisher class.
 */
UCLASS()
class EVENTBUSEDITOR_API UK2Node_EventBusAddPublisherValidated : public UK2Node_CallFunction
{
	GENERATED_BODY()

public:
	/** @brief Configures node to call `UEventBusBlueprintLibrary::AddPublisherValidated`. */
	UK2Node_EventBusAddPublisherValidated();

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(class FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

	/**
	 * @brief Builds dropdown options for DelegatePropertyName pin from selected publisher class.
	 */
	static void BuildDelegateOptions(const UEdGraphPin* DelegatePin, TArray<TSharedPtr<FName>>& OutOptions);

	/** @brief Name of object input pin used to infer publisher class for filtering. */
	static const FName PublisherObjPinName;
	/** @brief Name of delegate property pin that is rendered as filtered dropdown. */
	static const FName DelegatePropertyPinName;
};
