#pragma once

#include "EventBus/Typed/EventChannelDef.h"

#include "EventBusToyTags.h"
#include "ToyStatsPublisherComponent.h"

/** @brief Typed channel contract for toy health updates. */
NFL_DECLARE_EVENTBUS_CHANNEL(
	FToyHealthChangedChannel,
	FOnToyHealthChanged,
	UToyStatsPublisherComponent,
	TAG_Event_Toy_HealthChanged,
	OnToyHealthChanged
);

/** @brief Typed channel contract for toy stamina updates. */
NFL_DECLARE_EVENTBUS_CHANNEL(
	FToyStaminaChangedChannel,
	FOnToyStaminaChanged,
	UToyStatsPublisherComponent,
	TAG_Event_Toy_StaminaChanged,
	OnToyStaminaChanged
);
