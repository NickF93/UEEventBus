#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * @brief Produces filtered dropdown pins for EventBus custom K2 nodes.
 */
class FEventBusGraphPinFactory : public FGraphPanelPinFactory
{
public:
	/** @brief Creates custom `SGraphPinNameList` widgets for supported EventBus `FName` pins. */
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};
