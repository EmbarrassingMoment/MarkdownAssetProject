// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

using UnrealBuildTool;
using System.IO;

public class MarkdownAsset : ModuleRules
{
	public MarkdownAsset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "md4c"));
	}
}
