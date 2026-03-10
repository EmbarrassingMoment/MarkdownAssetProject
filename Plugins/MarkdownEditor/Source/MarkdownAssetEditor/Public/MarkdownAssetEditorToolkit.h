// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TimerManager.h"
#include "Editor.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"

class UMarkdownAsset;
class SMultiLineEditableTextBox;
class SWebBrowser;

/**
 * UI Commands for the Markdown editor toolbar and keyboard shortcuts.
 */
class FMarkdownEditorCommands : public TCommands<FMarkdownEditorCommands>
{
public:
	FMarkdownEditorCommands();

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> Bold;
	TSharedPtr<FUICommandInfo> Italic;
	TSharedPtr<FUICommandInfo> InsertCodeBlock;
	TSharedPtr<FUICommandInfo> Heading1;
	TSharedPtr<FUICommandInfo> Heading2;
	TSharedPtr<FUICommandInfo> Heading3;
	TSharedPtr<FUICommandInfo> BulletList;
	TSharedPtr<FUICommandInfo> NumberedList;
	TSharedPtr<FUICommandInfo> InsertTable;
	TSharedPtr<FUICommandInfo> Strikethrough;
	TSharedPtr<FUICommandInfo> HorizontalRule;
	TSharedPtr<FUICommandInfo> Blockquote;
};

class FMarkdownAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	virtual ~FMarkdownAssetEditorToolkit();

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
	void UpdatePreview();
	TSharedRef<SDockTab> SpawnTab_Main(const FSpawnTabArgs& Args);

	// Toolbar
	void RegisterToolbar();
	void BindCommands();
	void ExtendToolbar(FToolBarBuilder& ToolBarBuilder);
	void AddCenteredToolBarButton(FToolBarBuilder& ToolBarBuilder, const TSharedPtr<FUICommandInfo>& Command, const FText& Label);

	// Markdown formatting helpers
	void WrapSelectionWith(const FString& Prefix, const FString& Suffix);
	void InsertAtLineStart(const FString& Prefix);
	void InsertTextAtCursor(const FString& Text);

	// Command handlers
	void OnBold();
	void OnItalic();
	void OnStrikethrough();
	void OnInsertCodeBlock();
	void OnHeading1();
	void OnHeading2();
	void OnHeading3();
	void OnBulletList();
	void OnNumberedList();
	void OnInsertTable();
	void OnHorizontalRule();
	void OnBlockquote();

	FTimerHandle PreviewUpdateTimerHandle;

	UMarkdownAsset* MarkdownAsset;

	TSharedPtr<SMultiLineEditableTextBox> EditableTextBox;
	TSharedPtr<SWebBrowser> WebBrowserWidget;
	TSharedPtr<FUICommandList> ToolkitCommands;

	static const FName AppIdentifier;
	static const FName MainTabId;
};
