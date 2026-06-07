// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorModule.h"
#include "MarkdownAssetActions.h"
#include "MarkdownAssetThumbnailRenderer.h"
#include "MarkdownAsset.h"
#include "MarkitdownBlueprintLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/CoreDelegates.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FMarkdownAssetEditorModule"

DEFINE_LOG_CATEGORY_STATIC(LogMarkdownAssetEditor, Log, All);

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

	// Defer menu hookup until after all default-phase modules (including LevelEditor)
	// have registered their menus. Also handle hot-reload where the engine is already up.
	if (GIsRunning)
	{
		RegisterMenus();
	}
	else
	{
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FMarkdownAssetEditorModule::RegisterMenus);
	}
}

/** Adds 'Batch Convert to Markdown...' under Tools > Markdown. */
void FMarkdownAssetEditorModule::RegisterMenus()
{
	if (!UToolMenus::IsToolMenuUIEnabled())
	{
		UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("UToolMenus UI is not enabled; Markitdown menu entry will not appear."));
		return;
	}

	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (!ToolsMenu)
	{
		UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("ExtendMenu(LevelEditor.MainMenu.Tools) returned null; Markitdown menu entry will not appear."));
		return;
	}

	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(
		"MarkdownAssetPlugin", LOCTEXT("MarkdownToolsSection", "Markdown"));

	Section.AddMenuEntry(
		"MarkitdownBatchConvert",
		LOCTEXT("MarkitdownBatchConvertLabel", "Batch Convert to Markdown..."),
		LOCTEXT("MarkitdownBatchConvertTooltip", "Pick PDF/DOCX/PPTX/HTML files and create UMarkdownAsset entries via markitdown."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([]()
		{
			UMarkitdownBlueprintLibrary::RunBatchConvertWizard(FString());
		})));

	UE_LOG(LogMarkdownAssetEditor, Log, TEXT("Registered Tools > Markdown > Batch Convert to Markdown menu entry."));
}

/** Unregisters asset type actions when the module is unloaded. */
void FMarkdownAssetEditorModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
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
