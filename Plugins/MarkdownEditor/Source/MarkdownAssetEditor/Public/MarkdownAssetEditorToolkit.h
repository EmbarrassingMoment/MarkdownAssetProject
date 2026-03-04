#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UMarkdownAsset;
class SMultiLineEditableTextBox;
class SRichTextBlock;

class FMarkdownAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	void Initialize(UMarkdownAsset* InMarkdownAsset, const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost);

	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	void OnTextChanged(const FText& NewText);
	TSharedRef<SDockTab> SpawnTab_Main(const FSpawnTabArgs& Args);

	UMarkdownAsset* MarkdownAsset;

	TSharedPtr<SMultiLineEditableTextBox> EditableTextBox;
	TSharedPtr<SRichTextBlock> RichTextBlock;

	static const FName AppIdentifier;
	static const FName MainTabId;
};
