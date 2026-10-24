// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TimerManager.h"
#include "Editor.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Views/SListView.h"
#include "MarkdownOutline.h"
#include "MarkdownTextSearch.h"

class UMarkdownAsset;
class SMultiLineEditableTextBox;
class SEditableTextBox;
class SWebBrowser;
struct FWebNavigationRequest;
struct FAssetData;
struct FSlateFontInfo;
class ITableRow;
class STableViewBase;

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
	/** Command to create a texture asset from the clipboard image and insert a link. */
	TSharedPtr<FUICommandInfo> PasteImage;
	/** Command to open the Find bar. */
	TSharedPtr<FUICommandInfo> Find;
	/** Command to open the Find bar with the Replace row expanded. */
	TSharedPtr<FUICommandInfo> Replace;
	/** Command to jump to the next search match. */
	TSharedPtr<FUICommandInfo> FindNext;
	/** Command to jump to the previous search match. */
	TSharedPtr<FUICommandInfo> FindPrevious;
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

	/** Writes the given text into the asset (inside a transaction) when it differs, then schedules a preview refresh. */
	void ApplyTextToAsset(const FString& NewText);

	/** Schedules a debounced preview / outline refresh using the delay from the editor settings. */
	void SchedulePreviewUpdate();

	/** Re-applies theme / font preferences after Editor Preferences > Markdown Editor changes. */
	void HandleSettingsChanged();

	/** Returns the monospace editor font at the size configured in the editor settings. */
	static FSlateFontInfo GetEditorFont();

	/** Refreshes the HTML preview by parsing the current Markdown and loading it into the web browser widget. */
	void UpdatePreview();

	/** Spawns the main editor tab containing the text editor and HTML preview. */
	TSharedRef<SDockTab> SpawnTab_Main(const FSpawnTabArgs& Args);

	/** Spawns the outline tab listing the document's headings. */
	TSharedRef<SDockTab> SpawnTab_Outline(const FSpawnTabArgs& Args);

	/** Re-extracts headings from the current Markdown text and refreshes the outline list. */
	void RebuildOutline();

	/** Creates the row widget for a single outline entry. */
	TSharedRef<ITableRow> OnGenerateOutlineRow(TSharedPtr<FMarkdownHeading> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** Jumps the text editor and the HTML preview to the clicked heading. */
	void OnOutlineItemClicked(TSharedPtr<FMarkdownHeading> Item);

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
	/** Creates a texture asset from the clipboard image and inserts an image link. */
	void OnPasteImage();

	/** Inserts Markdown links for assets dropped from the Content Browser at the cursor. */
	void OnAssetsDropped(const TArray<FAssetData>& DroppedAssets);

	// ---- Find & Replace ----

	/** Builds the Find & Replace bar shown above the text editor. */
	TSharedRef<SWidget> BuildSearchBar();

	/** Shows the search bar, optionally with the replace row, seeded from the current selection. */
	void ShowSearchBar(bool bWithReplace);

	/** Hides the search bar and returns focus to the text editor. */
	void HideSearchBar();

	/** Opens the Find bar (Ctrl+F). */
	void OnFind();
	/** Opens the Find bar with the Replace row (Ctrl+H). */
	void OnReplace();
	/** Selects the next match (F3 / Enter in the find box). */
	void OnFindNext();
	/** Selects the previous match (Shift+F3 / Shift+Enter in the find box). */
	void OnFindPrevious();

	/** Re-runs the search when the query text changes. */
	void OnFindTextChanged(const FText& NewText);

	/** Handles Enter / Shift+Enter / Escape inside the find and replace boxes. */
	FReply OnSearchBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent, bool bIsReplaceBox);

	/** Recomputes the match list for the current query; optionally selects the match nearest the cursor. */
	void RefreshSearchMatches(bool bSelectNearest);

	/** Moves the selection to the match after (or before) the current selection, wrapping around. */
	void SelectAdjacentMatch(bool bForward);

	/** Selects the given match in the text editor and scrolls it into view. */
	void SelectMatch(int32 MatchIndex);

	/** Replaces the currently selected match with the replacement text, then selects the next one. */
	void ReplaceCurrentMatch();

	/** Replaces every match in the document with the replacement text. */
	void ReplaceAllMatches();

	/** Returns the "n of m" / "No results" label shown next to the find box. */
	FText GetSearchStatusText() const;

	/** Returns the flat editor text used for searching. */
	FString GetEditorText() const;

	/** Intercepts navigation in the preview browser to handle mdasset:// links. */
	bool HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request);

	/** Opens a linked Markdown asset by name via the Asset Registry. */
	void OpenLinkedMarkdownAsset(const FString& AssetName);

	/** Opens an Unreal asset referenced by a package / object path (e.g. /Game/Path/To/Asset). */
	void OpenLinkedUnrealAsset(const FString& ObjectPath);

	/** Opens a C++ class in the IDE or a Blueprint class in its asset editor by class name. */
	void OpenLinkedClass(const FString& ClassName);

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

	/** Delegate handle for the editor settings change notification. */
	FDelegateHandle SettingsChangedHandle;

	/** Search query box of the Find bar. */
	TSharedPtr<SEditableTextBox> FindTextBox;
	/** Replacement text box of the Find bar. */
	TSharedPtr<SEditableTextBox> ReplaceTextBox;
	/** Whether the Find bar is shown above the editor. */
	bool bSearchBarVisible = false;
	/** Whether the Replace row of the Find bar is expanded. */
	bool bReplaceRowVisible = false;
	/** Whether searches are case-sensitive. */
	bool bSearchMatchCase = false;
	/** Matches of the current query in the editor text, in document order. */
	TArray<FMarkdownTextMatch> SearchMatches;
	/** Index into SearchMatches of the currently selected match, or INDEX_NONE. */
	int32 CurrentMatchIndex = INDEX_NONE;

	/** Headings currently shown in the outline panel. */
	TArray<TSharedPtr<FMarkdownHeading>> OutlineItems;
	/** The list view widget backing the outline panel. */
	TSharedPtr<SListView<TSharedPtr<FMarkdownHeading>>> OutlineListView;

	/** Unique identifier for this editor application. */
	static const FName AppIdentifier;
	/** Tab ID for the main editor tab. */
	static const FName MainTabId;
	/** Tab ID for the outline tab. */
	static const FName OutlineTabId;
};
