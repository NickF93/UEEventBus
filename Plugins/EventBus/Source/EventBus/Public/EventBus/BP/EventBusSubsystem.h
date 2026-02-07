#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBus.h"

#include "EventBusSubsystem.generated.h"

class UEventBusRegistryAsset;

/**
 * @brief Game-instance host for the v2 EventBus runtime.
 */
UCLASS()
class EVENTBUS_API UEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** @brief Ensures EventBus runtime teardown happens during subsystem shutdown. */
	virtual void Deinitialize() override;

	/** @brief Returns mutable EventBus runtime owned by this subsystem. */
	NFL_EVENTBUS_NODISCARD
	Nfrrlib::EventBus::FEventBus& GetEventBus();
	/** @brief Returns active registry asset used for Blueprint allowlist validation. */
	NFL_EVENTBUS_NODISCARD
	const UEventBusRegistryAsset* GetRegistry() const;

	/** @brief Sets allowlist registry used by blueprint-validated bind operations. */
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void SetRegistry(UEventBusRegistryAsset* InRegistry);

private:
	Nfrrlib::EventBus::FEventBus EventBus;

	UPROPERTY(EditAnywhere, Category = "EventBus")
	TObjectPtr<UEventBusRegistryAsset> Registry;
};
