#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "EventBus/Core/EventBusAttributes.h"

#include "EventBusTestObjects.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEventBusTestFloatDelegate, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEventBusTestPairDelegate, float, First, int32, Second);

UCLASS()
class EVENTBUS_API UEventBusTestPublisherObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Tests")
	FEventBusTestFloatDelegate OnValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Tests")
	FEventBusTestPairDelegate OnPairChanged;

	UFUNCTION()
	void EmitValue(float InValue);

	UFUNCTION()
	void EmitPair(float InFirst, int32 InSecond);
};

UCLASS()
class EVENTBUS_API UEventBusTestListenerObject : public UObject
{
	GENERATED_BODY()

public:
	int32 ValueCallCount = 0;
	int32 PairCallCount = 0;

	UFUNCTION()
	void OnValue(float InValue);

	UFUNCTION()
	void OnPair(float InFirst, int32 InSecond);

	UFUNCTION()
	void OnNoArgs();
};
