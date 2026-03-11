// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorModule.h"
#include "MarkdownAssetActions.h"
#include "MarkdownAssetThumbnailRenderer.h"
#include "MarkdownAsset.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ThumbnailRendering/ThumbnailManager.h"

#define LOCTEXT_NAMESPACE "FMarkdownAssetEditorModule"

/** Registers the Markdown asset category, type actions, and thumbnail renderer. */
void FMarkdownAssetEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Register custom asset category
	MarkdownAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Markdown")), LOCTEXT("MarkdownAssetCategory", "Markdown"));

	// Register asset actions
	MarkdownAssetActions = MakeShareable(new FMarkdownAssetActions(MarkdownAssetCategoryBit));
	AssetTools.RegisterAssetTypeActions(MarkdownAssetActions.ToSharedRef());

	// Register thumbnail renderer
	UThumbnailManager::Get().RegisterCustomRenderer(UMarkdownAsset::StaticClass(), UMarkdownAssetThumbnailRenderer::StaticClass());
}

/** Unregisters asset type actions when the module is unloaded. */
void FMarkdownAssetEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

		if (MarkdownAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(MarkdownAssetActions.ToSharedRef());
		}
	}

	MarkdownAssetActions.Reset();

	// Unregister thumbnail renderer
	if (UObjectInitialized())
	{
		UThumbnailManager::Get().UnregisterCustomRenderer(UMarkdownAsset::StaticClass());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMarkdownAssetEditorModule, MarkdownAssetEditor)
