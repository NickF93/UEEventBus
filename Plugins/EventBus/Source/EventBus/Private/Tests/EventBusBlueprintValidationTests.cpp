#include "Misc/AutomationTest.h"

#include "NativeGameplayTags.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/BP/EventBusRegistryAsset.h"
#include "Tests/EventBusTestObjects.h"

#if WITH_DEV_AUTOMATION_TESTS

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_BP, "EventBus.Test.BP");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_BP_Unknown, "EventBus.Test.BP.Unknown");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusBlueprintRegistryValidationTest,
	"EventBus.Blueprint.RegistryHistory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusBlueprintRegistryValidationTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	UEventBusRegistryAsset* Registry = NewObject<UEventBusRegistryAsset>();
	TestNotNull(TEXT("Registry asset created"), Registry);

	Registry->RecordPublisherBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestPublisherObject::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged));
	Registry->RecordListenerBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue));

	const TArray<FName> KnownFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass());
	TestEqual(TEXT("One recorded listener function is returned"), KnownFunctions.Num(), 1);
	TestTrue(TEXT("Recorded function exists in returned list"), KnownFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue)));

	const TArray<FName> UnknownChannelFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP_Unknown,
		UEventBusTestListenerObject::StaticClass());
	TestEqual(TEXT("Unknown channel has no recorded functions"), UnknownChannelFunctions.Num(), 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
