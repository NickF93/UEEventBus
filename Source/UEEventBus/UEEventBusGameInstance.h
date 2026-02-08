#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "UEEventBusGameInstance.generated.h"

/**
 * @brief Minimal game instance that initializes EventBus subsystem wiring.
 *
 * This class intentionally does not register channels. Channel registration
 * remains gameplay-driven (Blueprint or C++).
 */
UCLASS(BlueprintType, Blueprintable)
class UEEVENTBUS_API UUEEventBusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** @brief Initializes EventBus subsystem. */
	virtual void Init() override;
};
