#include "Tests/EventBusTestObjects.h"

/**
 * @brief Emits float delegate payload.
 */
void UEventBusTestPublisherObject::EmitValue(const float InValue)
{
	OnValueChanged.Broadcast(InValue);
}

/**
 * @brief Emits pair delegate payload.
 */
void UEventBusTestPublisherObject::EmitPair(const float InFirst, const int32 InSecond)
{
	OnPairChanged.Broadcast(InFirst, InSecond);
}

/**
 * @brief Increments value callback counter.
 */
void UEventBusTestListenerObject::OnValue(const float NFL_EVENTBUS_MAYBE_UNUSED InValue)
{
	++ValueCallCount;
}

/**
 * @brief Increments alternate value callback counter.
 */
void UEventBusTestListenerObject::OnValueAlt(const float NFL_EVENTBUS_MAYBE_UNUSED InValue)
{
	++ValueAltCallCount;
}

/**
 * @brief Increments pair callback counter.
 */
void UEventBusTestListenerObject::OnPair(
	const float NFL_EVENTBUS_MAYBE_UNUSED InFirst,
	const int32 NFL_EVENTBUS_MAYBE_UNUSED InSecond)
{
	++PairCallCount;
}

/**
 * @brief No-arg callback used to test signature mismatch validation.
 */
void UEventBusTestListenerObject::OnNoArgs()
{
}
