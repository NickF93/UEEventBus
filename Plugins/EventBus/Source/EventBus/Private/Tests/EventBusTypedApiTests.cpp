#include "Misc/AutomationTest.h"

#include "NativeGameplayTags.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBus.h"
#include "EventBus/Typed/EventChannelApi.h"
#include "EventBus/Typed/EventChannelDef.h"
#include "Tests/EventBusTestObjects.h"

#if WITH_DEV_AUTOMATION_TESTS

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_Typed, "EventBus.Test.Typed");

NFL_DECLARE_EVENTBUS_CHANNEL(
	FEventBusTypedTestChannel,
	FEventBusTestFloatDelegate,
	UEventBusTestPublisherObject,
	TAG_EventBus_Test_Typed,
	OnValueChanged
);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusTypedApiRegisterTest,
	"EventBus.Typed.RegisterAndPointerBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusTypedApiRegisterTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	TestTrue(TEXT("Typed channel register succeeds"), TEventChannelApi<FEventBusTypedTestChannel>::Register(Bus, true));
	TestFalse(TEXT("AddPublisher with null publisher fails"), TEventChannelApi<FEventBusTypedTestChannel>::AddPublisher(Bus, nullptr));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	TestTrue(TEXT("Typed AddPublisher succeeds"), TEventChannelApi<FEventBusTypedTestChannel>::AddPublisher(Bus, Publisher));
	TestTrue(TEXT("Typed AddListener succeeds"), NFL_EVENTBUS_ADD_LISTENER(Bus, FEventBusTypedTestChannel, Listener, UEventBusTestListenerObject, OnValue));

	Publisher->EmitValue(10.0f);
	TestEqual(TEXT("Listener called through typed route"), Listener->ValueCallCount, 1);

	TestTrue(TEXT("Typed RemoveListener succeeds"), NFL_EVENTBUS_REMOVE_LISTENER(Bus, FEventBusTypedTestChannel, Listener, UEventBusTestListenerObject, OnValue));
	Publisher->EmitValue(11.0f);
	TestEqual(TEXT("Listener is no longer called after remove"), Listener->ValueCallCount, 1);

	TestTrue(TEXT("Unregister channel succeeds"), Bus.UnregisterChannel(TAG_EventBus_Test_Typed));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
