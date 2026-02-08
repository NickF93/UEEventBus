// Copyright Epic Games, Inc. All Rights Reserved.

#include "EventBus.h"

#define LOCTEXT_NAMESPACE "FEventBusModule"

/**
 * @brief Initializes EventBus runtime module state.
 */
void FEventBusModule::StartupModule()
{
}

/**
 * @brief Releases EventBus runtime module state.
 */
void FEventBusModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEventBusModule, EventBus)
