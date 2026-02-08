using UnrealBuildTool;

public class EventBusEditor : ModuleRules
{
	public EventBusEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EventBus"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"UnrealEd",
				"GraphEditor",
				"BlueprintGraph",
				"KismetCompiler",
				"Kismet"
			});
	}
}
