#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ToyEventBusChannelsSubsystem.generated.h"

/**
 * @brief Toy game subsystem that pre-registers EventBus toy channels at game-instance startup.
 */
UCLASS()
class UEEVENTBUS_API UToyEventBusChannelsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** @brief Registers toy EventBus channels after EventBus runtime subsystem is initialized. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};

