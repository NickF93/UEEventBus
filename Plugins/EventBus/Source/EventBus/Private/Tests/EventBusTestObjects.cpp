#include "Tests/EventBusTestObjects.h"

void UEventBusTestPublisherObject::EmitValue(const float InValue)
{
	OnValueChanged.Broadcast(InValue);
}

void UEventBusTestPublisherObject::EmitPair(const float InFirst, const int32 InSecond)
{
	OnPairChanged.Broadcast(InFirst, InSecond);
}

void UEventBusTestListenerObject::OnValue(const float NFL_EVENTBUS_MAYBE_UNUSED InValue)
{
	++ValueCallCount;
}

void UEventBusTestListenerObject::OnPair(
	const float NFL_EVENTBUS_MAYBE_UNUSED InFirst,
	const int32 NFL_EVENTBUS_MAYBE_UNUSED InSecond)
{
	++PairCallCount;
}

void UEventBusTestListenerObject::OnNoArgs()
{
}
