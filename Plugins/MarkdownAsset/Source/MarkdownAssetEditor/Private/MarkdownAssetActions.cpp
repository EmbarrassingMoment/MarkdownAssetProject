// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetActions.h"
#include "MarkdownAsset.h"
#include "MarkdownAssetEditorToolkit.h"

#define LOCTEXT_NAMESPACE "MarkdownAssetActions"

/** Stores the asset category for later retrieval by the Content Browser. */
FMarkdownAssetActions::FMarkdownAssetActions(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

/** Returns "Markdown Text" as the display name for this asset type. */
FText FMarkdownAssetActions::GetName() const
{
	return LOCTEXT("MarkdownAssetName", "Markdown Text");
}

/** Returns a gray color to represent Markdown assets in the editor UI. */
FColor FMarkdownAssetActions::GetTypeColor() const
{
	return FColor(128, 128, 128); // Gray color
}

/** Returns UMarkdownAsset as the supported class. */
UClass* FMarkdownAssetActions::GetSupportedClass() const
{
	return UMarkdownAsset::StaticClass();
}

/** Returns the registered Markdown asset category. */
uint32 FMarkdownAssetActions::GetCategories()
{
	return AssetCategory;
}

/** Creates and initializes a FMarkdownAssetEditorToolkit for each selected Markdown asset. */
void FMarkdownAssetActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(*ObjIt);
		if (MarkdownAsset != nullptr)
		{
			TSharedRef<FMarkdownAssetEditorToolkit> EditorToolkit = MakeShareable(new FMarkdownAssetEditorToolkit());
			EditorToolkit->Initialize(MarkdownAsset, Mode, EditWithinLevelEditor);
		}
	}
}

#undef LOCTEXT_NAMESPACE
