// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

using UnrealBuildTool;

public class MarkdownAssetEditor : ModuleRules
{
	public MarkdownAssetEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MarkdownAsset"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Slate",
				"SlateCore",
				"InputCore",
				"AssetTools",
				"AppFramework",
				"WorkspaceMenuStructure",
				"WebBrowser"
			}
		);
	}
}
