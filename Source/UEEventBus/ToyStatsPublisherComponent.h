#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "ToyStatsPublisherComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToyHealthChanged, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToyStaminaChanged, float, NewStamina);

/**
 * @brief Toy publisher component used to validate EventBus v2 channel routing.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UEEVENTBUS_API UToyStatsPublisherComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UToyStatsPublisherComponent();

	/** @brief Broadcast when toy health changes. */
	UPROPERTY(BlueprintAssignable, Category = "Toy|EventBus")
	FOnToyHealthChanged OnToyHealthChanged;

	/** @brief Broadcast when toy stamina changes. */
	UPROPERTY(BlueprintAssignable, Category = "Toy|EventBus")
	FOnToyStaminaChanged OnToyStaminaChanged;

	/** @brief Sets health and broadcasts `OnToyHealthChanged`. */
	UFUNCTION(BlueprintCallable, Category = "Toy|EventBus")
	void SetHealth(float InHealth);

	/** @brief Sets stamina and broadcasts `OnToyStaminaChanged`. */
	UFUNCTION(BlueprintCallable, Category = "Toy|EventBus")
	void SetStamina(float InStamina);

protected:
	/** @brief Registers publishers into EventBus at startup. */
	virtual void BeginPlay() override;
	/** @brief Removes publishers from EventBus at teardown. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** @brief Local toy health sample value. */
	UPROPERTY(EditAnywhere, Category = "Toy|Stats")
	float Health = 100.0f;

	/** @brief Local toy stamina sample value. */
	UPROPERTY(EditAnywhere, Category = "Toy|Stats")
	float Stamina = 100.0f;
};
