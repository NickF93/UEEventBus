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

	Registry->PublisherHistory.AddDefaulted();
	Registry->ListenerHistory.AddDefaulted();

	Registry->RecordPublisherBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestPublisherObject::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged));
	Registry->RecordListenerBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue));

	TestEqual(TEXT("Invalid publisher history entries are pruned"), Registry->PublisherHistory.Num(), 1);
	TestEqual(TEXT("Invalid listener history entries are pruned"), Registry->ListenerHistory.Num(), 1);

	const TArray<FName> KnownFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass());
	TestEqual(TEXT("One recorded listener function is returned"), KnownFunctions.Num(), 1);
	TestTrue(TEXT("Recorded function exists in returned list"), KnownFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue)));

	const TArray<FName> UnknownChannelFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP_Unknown,
		UEventBusTestListenerObject::StaticClass());
	TestEqual(TEXT("Unknown channel has no recorded functions"), UnknownChannelFunctions.Num(), 0);

	Registry->RecordListenerBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestDerivedListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestDerivedListenerObject, OnDerivedValue));

	const TArray<FName> BaseClassKnownFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestListenerObject::StaticClass());
	TestFalse(TEXT("Base class query does not include derived-only function"), BaseClassKnownFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestDerivedListenerObject, OnDerivedValue)));

	const TArray<FName> DerivedClassKnownFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestDerivedListenerObject::StaticClass());
	TestTrue(TEXT("Derived class query includes derived-only function"), DerivedClassKnownFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestDerivedListenerObject, OnDerivedValue)));

	Registry->ResetHistory();
	TestEqual(TEXT("ResetHistory clears publisher history"), Registry->PublisherHistory.Num(), 0);
	TestEqual(TEXT("ResetHistory clears listener history"), Registry->ListenerHistory.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusBlueprintRegistryBoundedHistoryTest,
	"EventBus.Blueprint.RegistryHistoryBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusBlueprintRegistryBoundedHistoryTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	UEventBusRegistryAsset* Registry = NewObject<UEventBusRegistryAsset>();
	TestNotNull(TEXT("Registry asset created"), Registry);

	for (int32 Index = 0; Index < 512; ++Index)
	{
		FEventBusPublisherHistoryEntry Entry;
		Entry.ChannelTag = TAG_EventBus_Test_BP;
		Entry.PublisherClass = UEventBusTestPublisherObject::StaticClass();
		Entry.DelegatePropertyName = FName(*FString::Printf(TEXT("PreloadedPublisherDelegate_%d"), Index));
		Registry->PublisherHistory.Add(MoveTemp(Entry));
	}

	Registry->RecordPublisherBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestPublisherObject::StaticClass(),
		GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged));

	TestEqual(TEXT("Publisher history remains bounded"), Registry->PublisherHistory.Num(), 512);
	const bool bPublisherTailEntryPresent = Registry->PublisherHistory.ContainsByPredicate(
		[](const FEventBusPublisherHistoryEntry& Entry)
		{
			return Entry.DelegatePropertyName == GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
		});
	TestTrue(TEXT("Newest publisher record is retained after bounding"), bPublisherTailEntryPresent);

	Registry->ResetHistory();

	for (int32 Index = 0; Index < 512; ++Index)
	{
		FEventBusListenerHistoryEntry Entry;
		Entry.ChannelTag = TAG_EventBus_Test_BP;
		Entry.ListenerClass = UEventBusTestListenerObject::StaticClass();
		Entry.KnownFunctions.Add(FName(*FString::Printf(TEXT("PreloadedListenerFunction_%d"), Index)));
		Registry->ListenerHistory.Add(MoveTemp(Entry));
	}

	Registry->RecordListenerBinding(
		TAG_EventBus_Test_BP,
		UEventBusTestDerivedListenerObject::StaticClass(),
		GET_FUNCTION_NAME_CHECKED(UEventBusTestDerivedListenerObject, OnDerivedValue));

	TestEqual(TEXT("Listener history remains bounded"), Registry->ListenerHistory.Num(), 512);
	const TArray<FName> DerivedFunctions = Registry->GetKnownListenerFunctions(
		TAG_EventBus_Test_BP,
		UEventBusTestDerivedListenerObject::StaticClass());
	TestTrue(TEXT("Newest listener record is retained after bounding"),
		DerivedFunctions.Contains(GET_FUNCTION_NAME_CHECKED(UEventBusTestDerivedListenerObject, OnDerivedValue)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
