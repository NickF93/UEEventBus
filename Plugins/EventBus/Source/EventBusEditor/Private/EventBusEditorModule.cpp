#include "EventBusEditorModule.h"

#include "EdGraphUtilities.h"
#include "Modules/ModuleManager.h"

#include "Pins/EventBusGraphPinFactory.h"

/**
 * @brief Registers EventBus custom graph pin factory for filtered picker widgets.
 */
void FEventBusEditorModule::StartupModule()
{
	PinFactory = MakeShared<FEventBusGraphPinFactory>();
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
}

/**
 * @brief Unregisters EventBus custom graph pin factory during editor module shutdown.
 */
void FEventBusEditorModule::ShutdownModule()
{
	if (PinFactory.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("UnrealEd")))
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
	}

	PinFactory.Reset();
}

IMPLEMENT_MODULE(FEventBusEditorModule, EventBusEditor)
