#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

struct FGraphPanelPinFactory;

/**
 * @brief Editor module hosting EventBus Blueprint node customizations.
 */
class EVENTBUSEDITOR_API FEventBusEditorModule : public IModuleInterface
{
public:
	/** @brief Registers editor pin factory used by EventBus custom K2 nodes. */
	virtual void StartupModule() override;
	/** @brief Unregisters editor pin factory during module shutdown. */
	virtual void ShutdownModule() override;

private:
	/** @brief Shared pin factory instance lifetime-bound to this editor module. */
	TSharedPtr<FGraphPanelPinFactory> PinFactory;
};
