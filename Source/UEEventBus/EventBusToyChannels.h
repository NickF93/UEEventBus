#pragma once

#include "EventBus/Typed/EventChannelDef.h"

#include "EventBusToyTags.h"
#include "ToyStatsPublisherComponent.h"

NFL_DECLARE_EVENTBUS_CHANNEL(
	FToyHealthChangedChannel,
	FOnToyHealthChanged,
	UToyStatsPublisherComponent,
	TAG_Event_Toy_HealthChanged,
	OnToyHealthChanged
);

NFL_DECLARE_EVENTBUS_CHANNEL(
	FToyStaminaChangedChannel,
	FOnToyStaminaChanged,
	UToyStatsPublisherComponent,
	TAG_Event_Toy_StaminaChanged,
	OnToyStaminaChanged
);

