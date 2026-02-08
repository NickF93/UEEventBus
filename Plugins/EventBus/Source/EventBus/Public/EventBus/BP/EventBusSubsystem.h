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
	/** @brief Emits subsystem startup diagnostics for runtime tracing. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** @brief Ensures EventBus runtime teardown happens during subsystem shutdown. */
	virtual void Deinitialize() override;

	/** @brief Returns mutable EventBus runtime owned by this subsystem. */
	NFL_EVENTBUS_NODISCARD
	Nfrrlib::EventBus::FEventBus& GetEventBus();
	/** @brief Returns active runtime history store used by Blueprint APIs. */
	NFL_EVENTBUS_NODISCARD
	const UEventBusRegistryAsset* GetRuntimeRegistry() const;

private:
	/** @brief Core runtime orchestrator owned by this game-instance subsystem. */
	Nfrrlib::EventBus::FEventBus EventBus;

	/** @brief Transient runtime history store used by BP helpers and filtered picker nodes. */
	UPROPERTY(Transient)
	TObjectPtr<UEventBusRegistryAsset> RuntimeRegistry;
};
