#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "EventBusRegistryAsset.generated.h"

USTRUCT(BlueprintType)
struct EVENTBUS_API FEventBusPublisherRule
{
	GENERATED_BODY()

	/** @brief Channel rule scope. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	FGameplayTag ChannelTag;

	/** @brief Allowed publisher class (subclasses are accepted). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	TSubclassOf<UObject> PublisherClass;

	/** @brief Allowed multicast delegate property name on publisher class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	FName DelegatePropertyName = NAME_None;
};

USTRUCT(BlueprintType)
struct EVENTBUS_API FEventBusListenerRule
{
	GENERATED_BODY()

	/** @brief Channel rule scope. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	FGameplayTag ChannelTag;

	/** @brief Allowed listener class (subclasses are accepted). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	TSubclassOf<UObject> ListenerClass;

	/** @brief Allowed listener function names for this class/channel rule. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	TArray<FName> AllowedFunctions;
};

/**
 * @brief Governance asset used by Blueprint-facing API validation.
 */
UCLASS(BlueprintType)
class EVENTBUS_API UEventBusRegistryAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Returns true when publisher class and delegate property are allowlisted on channel. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EventBus")
	bool IsPublisherAllowed(const FGameplayTag& ChannelTag, UClass* PublisherClass, FName DelegatePropertyName) const;

	/** @brief Returns true when listener class/function are allowlisted on channel. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EventBus")
	bool IsListenerAllowed(const FGameplayTag& ChannelTag, UClass* ListenerClass, FName FunctionName) const;

	/** @brief Returns deduplicated/sorted allowlisted function names for a listener class on channel. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EventBus")
	TArray<FName> GetAllowedListenerFunctions(const FGameplayTag& ChannelTag, UClass* ListenerClass) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	TArray<FEventBusPublisherRule> PublisherRules;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EventBus")
	TArray<FEventBusListenerRule> ListenerRules;
};
