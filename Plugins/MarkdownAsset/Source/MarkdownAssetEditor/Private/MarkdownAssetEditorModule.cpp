// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorModule.h"
#include "MarkdownAssetActions.h"
#include "MarkdownAssetThumbnailRenderer.h"
#include "MarkdownAsset.h"
#include "MarkitdownBlueprintLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ToolMenus.h"

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

	// Register Tools menu entries once UToolMenus is up.
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMarkdownAssetEditorModule::RegisterMenus));
}

/** Adds 'Batch Convert to Markdown...' under Tools > Markdown. */
void FMarkdownAssetEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (!ToolsMenu)
	{
		return;
	}

	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(
		"Markdown", LOCTEXT("MarkdownToolsSection", "Markdown"));

	Section.AddMenuEntry(
		"MarkitdownBatchConvert",
		LOCTEXT("MarkitdownBatchConvertLabel", "Batch Convert to Markdown..."),
		LOCTEXT("MarkitdownBatchConvertTooltip", "Pick PDF/DOCX/PPTX/HTML files and create UMarkdownAsset entries via markitdown."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			UMarkitdownBlueprintLibrary::RunBatchConvertWizard(FString());
		})));
}

/** Unregisters asset type actions when the module is unloaded. */
void FMarkdownAssetEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

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
