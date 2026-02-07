#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "EventBus/Core/EventBusAttributes.h"

#include "EventBusTestObjects.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventBusTestFloatDelegate, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventBusTestPairDelegate, float, First, int32, Second);

/**
 * @brief Test publisher object exposing delegates used by EventBus automation tests.
 */
UCLASS()
class EVENTBUS_API UEventBusTestPublisherObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Tests")
	FEventBusTestFloatDelegate OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Tests")
	FEventBusTestPairDelegate OnPairChanged;

	/** @brief Broadcasts one-parameter test delegate. */
	UFUNCTION()
	void EmitValue(float InValue);

	/** @brief Broadcasts two-parameter test delegate. */
	UFUNCTION()
	void EmitPair(float InFirst, int32 InSecond);
};

/**
 * @brief Test listener object used to validate callback routing and unbinding behavior.
 */
UCLASS()
class EVENTBUS_API UEventBusTestListenerObject : public UObject
{
	GENERATED_BODY()

public:
	int32 ValueCallCount = 0;
	int32 ValueAltCallCount = 0;
	int32 PairCallCount = 0;

	/** @brief Callback compatible with FEventBusTestFloatDelegate. */
	UFUNCTION()
	void OnValue(float InValue);

	/** @brief Secondary callback compatible with FEventBusTestFloatDelegate. */
	UFUNCTION()
	void OnValueAlt(float InValue);

	/** @brief Callback compatible with FEventBusTestPairDelegate. */
	UFUNCTION()
	void OnPair(float InFirst, int32 InSecond);

	/** @brief Callback intentionally incompatible with delegate signatures for negative tests. */
	UFUNCTION()
	void OnNoArgs();
};
