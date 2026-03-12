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
	/** Initializes the command set with context name and style. */
	FMarkdownEditorCommands();

	/** Registers all Markdown formatting commands with their keyboard shortcuts. */
	virtual void RegisterCommands() override;

	/** Command to toggle bold formatting. */
	TSharedPtr<FUICommandInfo> Bold;
	/** Command to toggle italic formatting. */
	TSharedPtr<FUICommandInfo> Italic;
	/** Command to insert a fenced code block. */
	TSharedPtr<FUICommandInfo> InsertCodeBlock;
	/** Command to insert a level-1 heading. */
	TSharedPtr<FUICommandInfo> Heading1;
	/** Command to insert a level-2 heading. */
	TSharedPtr<FUICommandInfo> Heading2;
	/** Command to insert a level-3 heading. */
	TSharedPtr<FUICommandInfo> Heading3;
	/** Command to insert a bullet list item. */
	TSharedPtr<FUICommandInfo> BulletList;
	/** Command to insert a numbered list item. */
	TSharedPtr<FUICommandInfo> NumberedList;
	/** Command to insert a Markdown table. */
	TSharedPtr<FUICommandInfo> InsertTable;
	/** Command to toggle strikethrough formatting. */
	TSharedPtr<FUICommandInfo> Strikethrough;
	/** Command to insert a horizontal rule. */
	TSharedPtr<FUICommandInfo> HorizontalRule;
	/** Command to insert a blockquote. */
	TSharedPtr<FUICommandInfo> Blockquote;
};

/**
 * Custom asset editor toolkit for Markdown assets.
 * Provides a split-pane editor with a raw Markdown text editor on the left
 * and a live HTML preview on the right, along with a formatting toolbar.
 */
class FMarkdownAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	/** Destructor. Unregisters commands and clears the preview update timer. */
	virtual ~FMarkdownAssetEditorToolkit();

	/** Registers the main editor tab spawner with the tab manager. */
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/** Unregisters the main editor tab spawner from the tab manager. */
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/** Initializes the editor with the given Markdown asset, mode, and host. */
	void Initialize(UMarkdownAsset* InMarkdownAsset, const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost);

	/** Returns the internal toolkit name identifier. */
	virtual FName GetToolkitFName() const override;

	/** Returns the human-readable base name of the toolkit. */
	virtual FText GetBaseToolkitName() const override;

	/** Returns the tab prefix used in world-centric editing mode. */
	virtual FString GetWorldCentricTabPrefix() const override;

	/** Returns the tab color scale used in world-centric editing mode. */
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	/** Called when the user modifies text in the editor; updates the asset and schedules a preview refresh. */
	void OnTextChanged(const FText& NewText);

	/** Refreshes the HTML preview by parsing the current Markdown and loading it into the web browser widget. */
	void UpdatePreview();

	/** Spawns the main editor tab containing the text editor and HTML preview. */
	TSharedRef<SDockTab> SpawnTab_Main(const FSpawnTabArgs& Args);

	/** Registers the formatting toolbar extension. */
	void RegisterToolbar();

	/** Binds UI commands to their corresponding formatting handler methods. */
	void BindCommands();

	/** Populates the toolbar with formatting buttons. */
	void ExtendToolbar(FToolBarBuilder& ToolBarBuilder);

	/** Adds a centered button widget to the toolbar for the given command. */
	void AddCenteredToolBarButton(FToolBarBuilder& ToolBarBuilder, const TSharedPtr<FUICommandInfo>& Command, const FText& Label);

	/** Wraps the current text selection with the specified prefix and suffix strings. */
	void WrapSelectionWith(const FString& Prefix, const FString& Suffix);

	/** Inserts the given prefix at the beginning of the current line. */
	void InsertAtLineStart(const FString& Prefix);

	/** Inserts the given text at the current cursor position. */
	void InsertTextAtCursor(const FString& Text);

	/** Applies bold formatting to the selection. */
	void OnBold();
	/** Applies italic formatting to the selection. */
	void OnItalic();
	/** Applies strikethrough formatting to the selection. */
	void OnStrikethrough();
	/** Inserts a fenced code block at the cursor. */
	void OnInsertCodeBlock();
	/** Inserts a level-1 heading prefix at the cursor. */
	void OnHeading1();
	/** Inserts a level-2 heading prefix at the cursor. */
	void OnHeading2();
	/** Inserts a level-3 heading prefix at the cursor. */
	void OnHeading3();
	/** Inserts a bullet list item prefix at the cursor. */
	void OnBulletList();
	/** Inserts a numbered list item prefix at the cursor. */
	void OnNumberedList();
	/** Inserts a Markdown table template at the cursor. */
	void OnInsertTable();
	/** Inserts a horizontal rule at the cursor. */
	void OnHorizontalRule();
	/** Inserts a blockquote prefix at the cursor. */
	void OnBlockquote();

	/** Timer handle for debouncing preview updates after text changes. */
	FTimerHandle PreviewUpdateTimerHandle;

	/** The Markdown asset currently being edited. */
	TObjectPtr<UMarkdownAsset> MarkdownAsset;

	/** The text editor widget for editing raw Markdown. */
	TSharedPtr<SMultiLineEditableTextBox> EditableTextBox;
	/** The web browser widget for displaying the rendered HTML preview. */
	TSharedPtr<SWebBrowser> WebBrowserWidget;
	/** The command list that maps UI commands to formatting actions. */
	TSharedPtr<FUICommandList> ToolkitCommands;

	/** Unique identifier for this editor application. */
	static const FName AppIdentifier;
	/** Tab ID for the main editor tab. */
	static const FName MainTabId;
};
