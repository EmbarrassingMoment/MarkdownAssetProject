#include "MarkdownAssetEditorModule.h"
#include "MarkdownAssetActions.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"

#define LOCTEXT_NAMESPACE "FMarkdownAssetEditorModule"

void FMarkdownAssetEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// Register custom asset category
	MarkdownAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("Markdown")), LOCTEXT("MarkdownAssetCategory", "Markdown"));

	// Register asset actions
	MarkdownAssetActions = MakeShareable(new FMarkdownAssetActions(MarkdownAssetCategoryBit));
	AssetTools.RegisterAssetTypeActions(MarkdownAssetActions.ToSharedRef());
}

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
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMarkdownAssetEditorModule, MarkdownAssetEditor)
