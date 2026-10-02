// FPVDrone game target.

using UnrealBuildTool;
using System.Collections.Generic;

public class FPVDroneTarget : TargetRules
{
	public FPVDroneTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("FPVDrone");
	}
}
