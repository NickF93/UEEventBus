#include "Misc/AutomationTest.h"

#include "Async/Async.h"
#include "NativeGameplayTags.h"
#include "UObject/GarbageCollection.h"

#include "EventBus/Core/EventBusAttributes.h"
#include "EventBus/Core/EventBus.h"
#include "Tests/EventBusTestObjects.h"

#if WITH_DEV_AUTOMATION_TESTS

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_Core, "EventBus.Test.Core");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_SignatureKnown, "EventBus.Test.SignatureKnown");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_ListenerFirst, "EventBus.Test.ListenerFirst");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_RenameIdentity, "EventBus.Test.RenameIdentity");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_ThreadGuard, "EventBus.Test.ThreadGuard");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_DeadCleanup, "EventBus.Test.DeadCleanup");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_OwningMultiFunc, "EventBus.Test.OwningMultiFunc");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_NonOwningSelective, "EventBus.Test.NonOwningSelective");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EventBus_Test_RemovePublisherStopsDispatch, "EventBus.Test.RemovePublisherStopsDispatch");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusCoreRegisterUnregisterTest,
	"EventBus.Core.RegisterUnregisterAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusCoreRegisterUnregisterTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_Core;
	Registration.bOwnsPublisherDelegates = false;

	TestTrue(TEXT("RegisterChannel succeeds"), Bus.RegisterChannel(Registration));
	TestTrue(TEXT("Idempotent register with same ownership succeeds"), Bus.RegisterChannel(Registration));

	FChannelRegistration ConflictRegistration;
	ConflictRegistration.ChannelTag = TAG_EventBus_Test_Core;
	ConflictRegistration.bOwnsPublisherDelegates = true;
	TestFalse(TEXT("Conflicting ownership register fails"), Bus.RegisterChannel(ConflictRegistration));

	TestTrue(TEXT("Channel is registered"), Bus.IsChannelRegistered(TAG_EventBus_Test_Core));
	TestTrue(TEXT("UnregisterChannel succeeds"), Bus.UnregisterChannel(TAG_EventBus_Test_Core));
	TestFalse(TEXT("Channel is no longer registered"), Bus.IsChannelRegistered(TAG_EventBus_Test_Core));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusSignatureMismatchKnownChannelTest,
	"EventBus.Core.SignatureMismatchKnownChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusSignatureMismatchKnownChannelTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_SignatureKnown;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_SignatureKnown, Publisher, PublisherBinding));

	FListenerBinding BadListenerBinding;
	BadListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnNoArgs);
	TestFalse(TEXT("Mismatched listener function fails"), Bus.AddListener(TAG_EventBus_Test_SignatureKnown, Listener, BadListenerBinding));

	FListenerBinding GoodListenerBinding;
	GoodListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("Compatible listener function succeeds"), Bus.AddListener(TAG_EventBus_Test_SignatureKnown, Listener, GoodListenerBinding));

	Publisher->EmitValue(1.0f);
	TestEqual(TEXT("Compatible listener is invoked"), Listener->ValueCallCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusListenerFirstMismatchPublisherFailTest,
	"EventBus.Core.ListenerFirstMismatchPublisherFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusListenerFirstMismatchPublisherFailTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_ListenerFirst;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FListenerBinding EarlyListenerBinding;
	EarlyListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnNoArgs);
	TestTrue(TEXT("Listener-first registration succeeds before signature is known"), Bus.AddListener(TAG_EventBus_Test_ListenerFirst, Listener, EarlyListenerBinding));

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestFalse(TEXT("First publisher fails due incompatible existing listener"), Bus.AddPublisher(TAG_EventBus_Test_ListenerFirst, Publisher, PublisherBinding));

	TestTrue(TEXT("Removing mismatched listener succeeds"), Bus.RemoveListener(TAG_EventBus_Test_ListenerFirst, Listener, EarlyListenerBinding));
	TestTrue(TEXT("Publisher succeeds after mismatch listener is removed"), Bus.AddPublisher(TAG_EventBus_Test_ListenerFirst, Publisher, PublisherBinding));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusListenerIdentityRenameSafeTest,
	"EventBus.Core.ListenerIdentityRenameSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusListenerIdentityRenameSafeTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_RenameIdentity;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_RenameIdentity, Publisher, PublisherBinding));

	FListenerBinding ListenerBinding;
	ListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("AddListener succeeds"), Bus.AddListener(TAG_EventBus_Test_RenameIdentity, Listener, ListenerBinding));

	Publisher->EmitValue(1.0f);
	TestEqual(TEXT("Listener invoked before rename"), Listener->ValueCallCount, 1);

	Listener->Rename(TEXT("RenamedEventBusListener"));
	TestTrue(TEXT("RemoveListener succeeds after listener rename"), Bus.RemoveListener(TAG_EventBus_Test_RenameIdentity, Listener, ListenerBinding));

	Publisher->EmitValue(1.0f);
	TestEqual(TEXT("Listener is no longer invoked after remove"), Listener->ValueCallCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusThreadGuardIsChannelRegisteredTest,
	"EventBus.Core.ThreadGuardIsChannelRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusThreadGuardIsChannelRegisteredTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_ThreadGuard;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	const bool bOffThreadResult = Async(EAsyncExecution::ThreadPool, [&Bus]()
	{
		return Bus.IsChannelRegistered(TAG_EventBus_Test_ThreadGuard);
	}).Get();

	TestFalse(TEXT("IsChannelRegistered returns false off-thread"), bOffThreadResult);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusDeadListenerCleanupTest,
	"EventBus.Core.DeadListenerCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusDeadListenerCleanupTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_DeadCleanup;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();
	const TWeakObjectPtr<UEventBusTestListenerObject> ListenerWeak(Listener);
	Publisher->AddToRoot();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_DeadCleanup, Publisher, PublisherBinding));

	FListenerBinding ListenerBinding;
	ListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("AddListener succeeds"), Bus.AddListener(TAG_EventBus_Test_DeadCleanup, Listener, ListenerBinding));
	TestTrue(TEXT("Delegate is bound after listener add"), Publisher->OnValueChanged.IsBound());

	Listener->MarkAsGarbage();
	CollectGarbage(RF_NoFlags);
	TestTrue(TEXT("Listener weak pointer is stale after GC"), ListenerWeak.IsStale(/*bIncludingIfPendingKill=*/true, /*bThreadsafeTest=*/false));
	TestTrue(TEXT("Re-adding publisher succeeds after listener garbage mark"), Bus.AddPublisher(TAG_EventBus_Test_DeadCleanup, Publisher, PublisherBinding));
	TestFalse(TEXT("Stale listener callback is removed during cleanup"), Publisher->OnValueChanged.IsBound());
	Publisher->RemoveFromRoot();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusOwningModeMultiFunctionBindingTest,
	"EventBus.Core.OwningModeMultiFunctionBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusOwningModeMultiFunctionBindingTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_OwningMultiFunc;
	Registration.bOwnsPublisherDelegates = true;
	TestTrue(TEXT("Register owning channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_OwningMultiFunc, Publisher, PublisherBinding));

	FListenerBinding ListenerBindingA;
	ListenerBindingA.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("Add first listener function succeeds"), Bus.AddListener(TAG_EventBus_Test_OwningMultiFunc, Listener, ListenerBindingA));

	FListenerBinding ListenerBindingB;
	ListenerBindingB.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValueAlt);
	TestTrue(TEXT("Add second listener function succeeds"), Bus.AddListener(TAG_EventBus_Test_OwningMultiFunc, Listener, ListenerBindingB));

	Publisher->EmitValue(5.0f);
	TestEqual(TEXT("First function receives callback"), Listener->ValueCallCount, 1);
	TestEqual(TEXT("Second function receives callback"), Listener->ValueAltCallCount, 1);

	TestTrue(TEXT("Owning remove by object+function succeeds"), Bus.RemoveListener(TAG_EventBus_Test_OwningMultiFunc, Listener, ListenerBindingA));
	Publisher->EmitValue(7.0f);
	TestEqual(TEXT("All object callbacks removed in owning mode (first)"), Listener->ValueCallCount, 1);
	TestEqual(TEXT("All object callbacks removed in owning mode (second)"), Listener->ValueAltCallCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusNonOwningSelectiveRemovalTest,
	"EventBus.Core.NonOwningSelectiveRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusNonOwningSelectiveRemovalTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_NonOwningSelective;
	Registration.bOwnsPublisherDelegates = false;
	TestTrue(TEXT("Register non-owning channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_NonOwningSelective, Publisher, PublisherBinding));

	FListenerBinding ListenerBindingA;
	ListenerBindingA.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("Add first listener function succeeds"), Bus.AddListener(TAG_EventBus_Test_NonOwningSelective, Listener, ListenerBindingA));

	FListenerBinding ListenerBindingB;
	ListenerBindingB.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValueAlt);
	TestTrue(TEXT("Add second listener function succeeds"), Bus.AddListener(TAG_EventBus_Test_NonOwningSelective, Listener, ListenerBindingB));

	Publisher->EmitValue(1.0f);
	TestEqual(TEXT("First function receives callback"), Listener->ValueCallCount, 1);
	TestEqual(TEXT("Second function receives callback"), Listener->ValueAltCallCount, 1);

	TestTrue(TEXT("Remove first listener function succeeds"), Bus.RemoveListener(TAG_EventBus_Test_NonOwningSelective, Listener, ListenerBindingA));
	Publisher->EmitValue(2.0f);
	TestEqual(TEXT("First function no longer receives callbacks"), Listener->ValueCallCount, 1);
	TestEqual(TEXT("Second function still receives callbacks"), Listener->ValueAltCallCount, 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEventBusRemovePublisherStopsDispatchTest,
	"EventBus.Core.RemovePublisherStopsDispatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventBusRemovePublisherStopsDispatchTest::RunTest(const FString& NFL_EVENTBUS_MAYBE_UNUSED Parameters)
{
	using namespace Nfrrlib::EventBus;

	FEventBus Bus;
	FChannelRegistration Registration;
	Registration.ChannelTag = TAG_EventBus_Test_RemovePublisherStopsDispatch;
	TestTrue(TEXT("Register channel succeeds"), Bus.RegisterChannel(Registration));

	UEventBusTestPublisherObject* Publisher = NewObject<UEventBusTestPublisherObject>();
	UEventBusTestListenerObject* Listener = NewObject<UEventBusTestListenerObject>();

	FPublisherBinding PublisherBinding;
	PublisherBinding.DelegatePropertyName = GET_MEMBER_NAME_CHECKED(UEventBusTestPublisherObject, OnValueChanged);
	TestTrue(TEXT("AddPublisher succeeds"), Bus.AddPublisher(TAG_EventBus_Test_RemovePublisherStopsDispatch, Publisher, PublisherBinding));

	FListenerBinding ListenerBinding;
	ListenerBinding.FunctionName = GET_FUNCTION_NAME_CHECKED(UEventBusTestListenerObject, OnValue);
	TestTrue(TEXT("AddListener succeeds"), Bus.AddListener(TAG_EventBus_Test_RemovePublisherStopsDispatch, Listener, ListenerBinding));

	Publisher->EmitValue(1.0f);
	TestEqual(TEXT("Listener receives callback before publisher removal"), Listener->ValueCallCount, 1);

	TestTrue(TEXT("RemovePublisher succeeds"), Bus.RemovePublisher(TAG_EventBus_Test_RemovePublisherStopsDispatch, Publisher));
	TestFalse(TEXT("Publisher delegate has no EventBus callback after removal"), Publisher->OnValueChanged.IsBound());

	Publisher->EmitValue(2.0f);
	TestEqual(TEXT("Listener is not called after publisher removal"), Listener->ValueCallCount, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
