// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetActions.h"
#include "MarkdownAsset.h"
#include "MarkdownAssetEditorToolkit.h"

FMarkdownAssetActions::FMarkdownAssetActions(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FMarkdownAssetActions::GetName() const
{
	return FText::FromString("Markdown Text");
}

FColor FMarkdownAssetActions::GetTypeColor() const
{
	return FColor(128, 128, 128); // Gray color
}

UClass* FMarkdownAssetActions::GetSupportedClass() const
{
	return UMarkdownAsset::StaticClass();
}

uint32 FMarkdownAssetActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

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
