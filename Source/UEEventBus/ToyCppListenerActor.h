#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ToyCppListenerActor.generated.h"

/**
 * @brief Toy C++ listener actor bound to EventBus v2 typed channels.
 */
UCLASS()
class UEEVENTBUS_API AToyCppListenerActor : public AActor
{
	GENERATED_BODY()

protected:
	/** @brief Registers typed listener callbacks into EventBus. */
	virtual void BeginPlay() override;
	/** @brief Removes typed listener callbacks from EventBus. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** @brief Handles toy health updates coming from EventBus. */
	UFUNCTION()
	void OnHealthChanged(float NewHealth);

	/** @brief Handles toy stamina updates coming from EventBus. */
	UFUNCTION()
	void OnStaminaChanged(float NewStamina);
};
