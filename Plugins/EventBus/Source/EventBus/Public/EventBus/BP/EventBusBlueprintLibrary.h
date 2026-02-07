#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "EventBus/Core/EventBusAttributes.h"

#include "EventBusBlueprintLibrary.generated.h"

class UEventBusSubsystem;

/**
 * @brief Blueprint facade for the v2 EventBus runtime.
 */
UCLASS()
class EVENTBUS_API UEventBusBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** @brief Registers one channel with ownership policy from Blueprint. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool RegisterChannel(UObject* WorldContextObject, FGameplayTag ChannelTag, bool bOwnsPublisherDelegates);

	/** @brief Unregisters one channel and unbinds its tracked callbacks. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool UnregisterChannel(UObject* WorldContextObject, FGameplayTag ChannelTag);

	/** @brief Adds an allowlisted publisher delegate binding to one channel. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool AddPublisherValidated(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* PublisherObj, FName DelegatePropertyName);

	/** @brief Removes one publisher from one channel. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool RemovePublisher(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* PublisherObj);

	/** @brief Adds an allowlisted listener function binding to one channel. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool AddListenerValidated(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* ListenerObj, FName FunctionName);

	/** @brief Removes one listener function binding from one channel. */
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static bool RemoveListener(UObject* WorldContextObject, FGameplayTag ChannelTag, UObject* ListenerObj, FName FunctionName);

	/** @brief Returns allowlisted listener functions for a channel/class pair. */
	UFUNCTION(BlueprintPure, Category = "EventBus", meta = (WorldContext = "WorldContextObject"))
	static TArray<FName> GetAllowedListenerFunctions(UObject* WorldContextObject, FGameplayTag ChannelTag, TSubclassOf<UObject> ListenerClass);

private:
	/** @brief Resolves EventBus subsystem from world context for all static Blueprint API entry points. */
	NFL_EVENTBUS_NODISCARD
	static UEventBusSubsystem* ResolveSubsystem(UObject* WorldContextObject);
};
