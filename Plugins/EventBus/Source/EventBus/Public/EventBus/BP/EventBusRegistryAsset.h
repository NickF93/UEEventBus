#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "EventBusRegistryAsset.generated.h"

USTRUCT(BlueprintType)
struct EVENTBUS_API FEventBusPublisherHistoryEntry
{
	GENERATED_BODY()

	/** @brief Channel for this publisher binding history entry. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	FGameplayTag ChannelTag;

	/** @brief Publisher class recorded for this channel/delegate binding. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	TSubclassOf<UObject> PublisherClass;

	/** @brief Publisher multicast delegate property name recorded for this binding. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	FName DelegatePropertyName = NAME_None;
};

USTRUCT(BlueprintType)
struct EVENTBUS_API FEventBusListenerHistoryEntry
{
	GENERATED_BODY()

	/** @brief Channel for this listener binding history entry. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	FGameplayTag ChannelTag;

	/** @brief Listener class recorded for this channel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	TSubclassOf<UObject> ListenerClass;

	/** @brief Listener function names recorded for this class/channel pair. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EventBus")
	TArray<FName> KnownFunctions;
};

/**
 * @brief Runtime history registry for blueprint channel/publisher/listener bindings.
 *
 * This object is not used as a manual rule table. It is populated dynamically
 * at runtime when successful binds occur.
 */
UCLASS(BlueprintType)
class EVENTBUS_API UEventBusRegistryAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Records one publisher binding into runtime history. */
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void RecordPublisherBinding(const FGameplayTag& ChannelTag, UClass* PublisherClass, FName DelegatePropertyName);

	/** @brief Records one listener function binding into runtime history. */
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void RecordListenerBinding(const FGameplayTag& ChannelTag, UClass* ListenerClass, FName FunctionName);

	/** @brief Returns deduplicated/sorted listener functions recorded for a class on a channel. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EventBus")
	TArray<FName> GetKnownListenerFunctions(const FGameplayTag& ChannelTag, UClass* ListenerClass) const;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "EventBus|History")
	TArray<FEventBusPublisherHistoryEntry> PublisherHistory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "EventBus|History")
	TArray<FEventBusListenerHistoryEntry> ListenerHistory;
};
