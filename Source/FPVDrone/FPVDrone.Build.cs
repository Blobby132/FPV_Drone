// FPVDrone runtime module.

using UnrealBuildTool;

public class FPVDrone : ModuleRules
{
	public FPVDrone(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Code lives in sub-folders (Flight/, Input/, UI/, ...) and is included relative to
		// the module root, e.g. #include "Flight/FPVFlightController.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			// Chaos physics-thread API (sim callback object, rigid body handle).
			"PhysicsCore",
			"Chaos"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			// Settings persistence (JSON file in Saved/).
			"Json",
			"JsonUtilities"
		});
	}
}
