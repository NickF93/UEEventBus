// Copyright Epic Games, Inc. All Rights Reserved.

#include "EventBus.h"

#define LOCTEXT_NAMESPACE "FEventBusModule"

void FEventBusModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FEventBusModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// this function is called before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEventBusModule, EventBus)
