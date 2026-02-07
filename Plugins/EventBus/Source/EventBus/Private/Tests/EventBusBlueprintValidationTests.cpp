#include "Misc/AutomationTest.h"

#include "NativeGameplayTags.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/BP/EventBusRegistryAsset.h"
#include "Tests/EventBusTestObjects.h"

#if WITH_DEV_AUTOMATION_TESTS

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_BP, "EventBus.Test.BP");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusBlueprintRegistryValidationTest,
	"EventBus.Blueprint.RegistryValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusBlueprintRegistryValidationTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	UEventBusRegistryAsset* Registry = NewObject<UEventBusRegistryAsset>();
	TestNotNull(TEXT("Registry asset created"), Registry);

	FEventBusPublisherRule PublisherRule;
	PublisherRule.ChannelTag = TAG_EventBus_Test_BP;
	PublisherRule.PublisherClass = UEventBusTestPublisherObject::StaticClass();
	PublisherRule.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	Registry->PublisherRules.Add(PublisherRule);

	FEventBusListenerRule ListenerRule;
	ListenerRule.ChannelTag = TAG_EventBus_Test_BP;
	ListenerRule.ListenerClass = UEventBusTestListenerObject::StaticClass();
	ListenerRule.AllowedFunctions.Add(GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue));
	Registry->ListenerRules.Add(ListenerRule);

	const bool bPublisherAllowed = Registry->IsPublisherAllowed(
		TAG_EventBus_Test_BP,
		UEventBusTestPublisherObject::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged));
	TestTrue(TEXT("Publisher allowlist positive case succeeds"), bPublisherAllowed);

	const bool bWrongPublisherPropertyDenied = Registry->IsPublisherAllowed(
		TAG_EventBus_Test_BP,
		UEventBusTestPublisherObject::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnPairChanged));
	TestFalse(TEXT("Publisher wrong property is denied"), bWrongPublisherPropertyDenied);

	const bool bListenerAllowed = Registry->IsListenerAllowed(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue));
	TestTrue(TEXT("Listener allowlist positive case succeeds"), bListenerAllowed);

	const bool bWrongListenerFunctionDenied = Registry->IsListenerAllowed(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnNoArgs));
	TestFalse(TEXT("Listener wrong function is denied"), bWrongListenerFunctionDenied);

	const TArray<FName> AllowedFunctions = Registry->GetAllowedListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass());
	TestEqual(TEXT("One allowlisted listener function is returned"), AllowedFunctions.Num(), 1);
	TestTrue(TEXT("Allowlisted function exists in returned list"), AllowedFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
