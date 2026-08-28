using UnrealBuildTool;
using System.Collections.Generic;

public class L20260713_Day03ServerTarget : TargetRules
{
	public L20260713_Day03ServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange(new string[] { "L20260713_Day03" });
	}
}
