// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * @brief Runtime module entry point for EventBus plugin.
 */
class FEventBusModule : public IModuleInterface
{
public:
	/** @brief Called when module is loaded by Unreal module manager. */
	virtual void StartupModule() override;
	/** @brief Called when module is unloading during shutdown/hot-reload. */
	virtual void ShutdownModule() override;
};
