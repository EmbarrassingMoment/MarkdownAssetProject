// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorToolkit.h"
#include "MarkdownAsset.h"
#include "MarkdownEditorSettings.h"
#include "MarkdownImageUtils.h"
#include "MarkdownLinkUtils.h"
#include "MarkdownOutline.h"
#include "MarkdownPreviewUtils.h"
#include "MarkdownTextSearch.h"
#include "AssetRegistry/AssetData.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/Texture2D.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Styling/CoreStyle.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "SWebBrowser.h"
#include "Misc/Base64.h"
#include "ScopedTransaction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Internationalization/Regex.h"
#include "SourceCodeNavigation.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "Misc/EngineVersionComparison.h"
#if UE_VERSION_OLDER_THAN(5, 6, 0)
#include "UObject/MetaData.h"
#endif
#include "Misc/PackageName.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "MarkdownAssetEditor"

DEFINE_LOG_CATEGORY_STATIC(LogMarkdownAssetEditor, Log, All);

// ---- SMarkdownAssetDropTarget ----

DECLARE_DELEGATE_OneParam(FOnMarkdownAssetsDropped, const TArray<FAssetData>&);

/**
 * Wraps the text editor so that assets dragged from the Content Browser can be
 * dropped onto it. SMultiLineEditableText does not handle drag & drop itself,
 * so the events bubble up to this widget.
 */
class SMarkdownAssetDropTarget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMarkdownAssetDropTarget) {}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_EVENT(FOnMarkdownAssetsDropped, OnAssetsDropped)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		OnAssetsDropped = InArgs._OnAssetsDropped;
		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		if (TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>())
		{
			AssetOp->SetToolTip(
				LOCTEXT("DropAssetLinkTooltip", "Insert a Markdown link to this asset at the cursor"),
				FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")));
		}
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		if (TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>())
		{
			AssetOp->ResetToDefaultToolTip();
		}
	}

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		return DragDropEvent.GetOperationAs<FAssetDragDropOp>().IsValid() ? FReply::Handled() : FReply::Unhandled();
	}

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		if (TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>())
		{
			AssetOp->ResetToDefaultToolTip();
			OnAssetsDropped.ExecuteIfBound(AssetOp->GetAssets());
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

private:
	FOnMarkdownAssetsDropped OnAssetsDropped;
};

// ---- FMarkdownEditorCommands ----

FMarkdownEditorCommands::FMarkdownEditorCommands()
	: TCommands<FMarkdownEditorCommands>(
		TEXT("MarkdownEditor"),
		LOCTEXT("MarkdownEditorCommands", "Markdown Editor"),
		NAME_None,
		FAppStyle::GetAppStyleSetName()
	)
{
}

void FMarkdownEditorCommands::RegisterCommands()
{
	UI_COMMAND(Bold, "Bold", "Wrap selection with bold markers (**)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::B));
	UI_COMMAND(Italic, "Italic", "Wrap selection with italic markers (*)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::I));
	UI_COMMAND(Strikethrough, "Strikethrough", "Wrap selection with strikethrough markers (~~)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::X));
	UI_COMMAND(InsertCodeBlock, "Code Block", "Insert a fenced code block", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::C));
	UI_COMMAND(Heading1, "H1", "Insert heading level 1", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::One));
	UI_COMMAND(Heading2, "H2", "Insert heading level 2", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Two));
	UI_COMMAND(Heading3, "H3", "Insert heading level 3", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::Three));
	UI_COMMAND(BulletList, "Bullet List", "Insert a bullet list item", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::U));
	UI_COMMAND(NumberedList, "Numbered List", "Insert a numbered list item", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::O));
	UI_COMMAND(InsertTable, "Table", "Insert a Markdown table", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(HorizontalRule, "Horizontal Rule", "Insert a horizontal rule", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(Blockquote, "Quote", "Insert a blockquote", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::Q));
	UI_COMMAND(PasteImage, "Paste Image", "Create a texture asset from the clipboard image and insert an image link", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::V));
	UI_COMMAND(Find, "Find", "Search the Markdown text", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	UI_COMMAND(Replace, "Replace", "Search and replace in the Markdown text", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::H));
	UI_COMMAND(FindNext, "Find Next", "Select the next search match", EUserInterfaceActionType::Button, FInputChord(EKeys::F3));
	UI_COMMAND(FindPrevious, "Find Previous", "Select the previous search match", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::F3));
}

// ---- FMarkdownAssetEditorToolkit ----

const FName FMarkdownAssetEditorToolkit::AppIdentifier(TEXT("MarkdownAssetEditorApp"));
const FName FMarkdownAssetEditorToolkit::MainTabId(TEXT("MarkdownAssetEditor_MainTab"));
const FName FMarkdownAssetEditorToolkit::OutlineTabId(TEXT("MarkdownAssetEditor_OutlineTab"));

FMarkdownAssetEditorToolkit::~FMarkdownAssetEditorToolkit()
{
	FMarkdownEditorCommands::Unregister();

	if (SettingsChangedHandle.IsValid() && UObjectInitialized())
	{
		GetMutableDefault<UMarkdownEditorSettings>()->OnSettingsChanged.Remove(SettingsChangedHandle);
		SettingsChangedHandle.Reset();
	}

	if (GEditor)
	{
		GEditor->GetTimerManager()->ClearTimer(PreviewUpdateTimerHandle);
	}
}

void FMarkdownAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(MainTabId, FOnSpawnTab::CreateSP(this, &FMarkdownAssetEditorToolkit::SpawnTab_Main))
		.SetDisplayName(LOCTEXT("MainTab", "Markdown Editor"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner(OutlineTabId, FOnSpawnTab::CreateSP(this, &FMarkdownAssetEditorToolkit::SpawnTab_Outline))
		.SetDisplayName(LOCTEXT("OutlineTab", "Outline"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FMarkdownAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(MainTabId);
	InTabManager->UnregisterTabSpawner(OutlineTabId);
}

void FMarkdownAssetEditorToolkit::Initialize(UMarkdownAsset* InMarkdownAsset, const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost)
{
	MarkdownAsset = InMarkdownAsset;

	FMarkdownEditorCommands::Register();
	BindCommands();

	// Create the layout (v2 adds the outline panel to the left of the main editor)
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MarkdownAssetEditor_Layout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.18f)
				->AddTab(OutlineTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.82f)
				->SetHideTabWell(true)
				->AddTab(MainTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(
		Mode,
		InitToolkitHost,
		AppIdentifier,
		StandaloneDefaultLayout,
		true, /*bCreateDefaultStandaloneMenu*/
		true, /*bCreateDefaultToolbar*/
		InMarkdownAsset
	);

	RegisterToolbar();
	RegenerateMenusAndToolbars();

	SettingsChangedHandle = GetMutableDefault<UMarkdownEditorSettings>()->OnSettingsChanged.AddSP(
		this, &FMarkdownAssetEditorToolkit::HandleSettingsChanged);
}

FName FMarkdownAssetEditorToolkit::GetToolkitFName() const
{
	return FName("MarkdownAssetEditor");
}

FText FMarkdownAssetEditorToolkit::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "Markdown Asset Editor");
}

FString FMarkdownAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("MarkdownAsset");
}

FLinearColor FMarkdownAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

// ---- Text Change & Preview ----

void FMarkdownAssetEditorToolkit::OnTextChanged(const FText& NewText)
{
	ApplyTextToAsset(NewText.ToString());
}

void FMarkdownAssetEditorToolkit::ApplyTextToAsset(const FString& NewText)
{
	if (!MarkdownAsset)
	{
		return;
	}

	// Programmatic SetText calls (Replace All, line-prefix insertion) and the
	// widget's own change notification can both land here; only record a
	// transaction when the text actually differs.
	if (MarkdownAsset->RawMarkdownText != NewText)
	{
		{
			FScopedTransaction Transaction(LOCTEXT("EditMarkdownText", "Edit Markdown Text"));
			MarkdownAsset->Modify();
			MarkdownAsset->RawMarkdownText = NewText;
		}
		MarkdownAsset->MarkPackageDirty();
	}

	SchedulePreviewUpdate();
}

void FMarkdownAssetEditorToolkit::SchedulePreviewUpdate()
{
	if (!GEditor)
	{
		return;
	}

	const UMarkdownEditorSettings* Settings = GetDefault<UMarkdownEditorSettings>();
	const float Delay = Settings ? Settings->PreviewUpdateDelay : 0.3f;

	GEditor->GetTimerManager()->ClearTimer(PreviewUpdateTimerHandle);
	if (Delay <= KINDA_SMALL_NUMBER)
	{
		UpdatePreview();
		return;
	}

	GEditor->GetTimerManager()->SetTimer(
		PreviewUpdateTimerHandle,
		FTimerDelegate::CreateSP(this, &FMarkdownAssetEditorToolkit::UpdatePreview),
		Delay,
		false
	);
}

void FMarkdownAssetEditorToolkit::HandleSettingsChanged()
{
	if (EditableTextBox.IsValid())
	{
		EditableTextBox->SetFont(GetEditorFont());
	}

	// Theme changes need a full preview rebuild.
	UpdatePreview();
}

FSlateFontInfo FMarkdownAssetEditorToolkit::GetEditorFont()
{
	const UMarkdownEditorSettings* Settings = GetDefault<UMarkdownEditorSettings>();
	const int32 FontSize = Settings ? FMath::Clamp(Settings->EditorFontSize, 6, 48) : 12;
	return FCoreStyle::GetDefaultFontStyle("Mono", FontSize);
}

void FMarkdownAssetEditorToolkit::UpdatePreview()
{
	RebuildOutline();

	if (MarkdownAsset && WebBrowserWidget.IsValid())
	{
		FString ParsedHtml = MarkdownAsset->GetParsedHTML();
		ParsedHtml = MarkdownLinkUtils::MarkBrokenLinks(ParsedHtml);
		ParsedHtml = MarkdownOutline::InjectHeadingAnchors(ParsedHtml);
		ParsedHtml = MarkdownImageUtils::EmbedTextureImages(ParsedHtml);

		MarkdownPreviewUtils::FStyledHtmlOptions Options;
		if (const UMarkdownEditorSettings* Settings = GetDefault<UMarkdownEditorSettings>())
		{
			Options.Theme = Settings->PreviewTheme;
		}
		FString StyledHtml = MarkdownPreviewUtils::GenerateStyledHtml(ParsedHtml, Options);

		// Explicitly convert FString (UTF-16) to UTF-8 bytes before Base64 encoding
		FTCHARToUTF8 Utf8Html(*StyledHtml);
		FString Base64Html = FBase64::Encode((uint8*)Utf8Html.Get(), Utf8Html.Length());
		FString DataUrl = FString::Printf(TEXT("data:text/html;base64,%s"), *Base64Html);

		WebBrowserWidget->LoadURL(DataUrl);
	}
}

// ---- Toolbar ----

void FMarkdownAssetEditorToolkit::BindCommands()
{
	ToolkitCommands = MakeShareable(new FUICommandList);
	const FMarkdownEditorCommands& Commands = FMarkdownEditorCommands::Get();

	ToolkitCommands->MapAction(Commands.Bold, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnBold));
	ToolkitCommands->MapAction(Commands.Italic, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnItalic));
	ToolkitCommands->MapAction(Commands.Strikethrough, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnStrikethrough));
	ToolkitCommands->MapAction(Commands.InsertCodeBlock, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnInsertCodeBlock));
	ToolkitCommands->MapAction(Commands.Heading1, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnHeading1));
	ToolkitCommands->MapAction(Commands.Heading2, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnHeading2));
	ToolkitCommands->MapAction(Commands.Heading3, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnHeading3));
	ToolkitCommands->MapAction(Commands.BulletList, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnBulletList));
	ToolkitCommands->MapAction(Commands.NumberedList, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnNumberedList));
	ToolkitCommands->MapAction(Commands.InsertTable, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnInsertTable));
	ToolkitCommands->MapAction(Commands.HorizontalRule, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnHorizontalRule));
	ToolkitCommands->MapAction(Commands.Blockquote, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnBlockquote));
	ToolkitCommands->MapAction(Commands.PasteImage, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnPasteImage));
	ToolkitCommands->MapAction(Commands.Find, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnFind));
	ToolkitCommands->MapAction(Commands.Replace, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnReplace));
	ToolkitCommands->MapAction(Commands.FindNext, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnFindNext));
	ToolkitCommands->MapAction(Commands.FindPrevious, FExecuteAction::CreateSP(this, &FMarkdownAssetEditorToolkit::OnFindPrevious));
}

void FMarkdownAssetEditorToolkit::RegisterToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		ToolkitCommands,
		FToolBarExtensionDelegate::CreateSP(this, &FMarkdownAssetEditorToolkit::ExtendToolbar)
	);

	AddToolbarExtender(ToolbarExtender);
}

void FMarkdownAssetEditorToolkit::AddCenteredToolBarButton(FToolBarBuilder& ToolBarBuilder, const TSharedPtr<FUICommandInfo>& Command, const FText& Label)
{
	ToolBarBuilder.AddWidget(
		SNew(SBox)
		.WidthOverride(50.0f)
		.HeightOverride(28.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.OnClicked_Lambda([this, Command]() -> FReply
			{
				if (ToolkitCommands.IsValid())
				{
					ToolkitCommands->ExecuteAction(Command.ToSharedRef());
				}
				return FReply::Handled();
			})
			.ToolTipText(Command->GetDescription())
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
				.Justification(ETextJustify::Center)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
		],
		FName(*Label.ToString())
	);
}

void FMarkdownAssetEditorToolkit::ExtendToolbar(FToolBarBuilder& ToolBarBuilder)
{
	const FMarkdownEditorCommands& Commands = FMarkdownEditorCommands::Get();

	ToolBarBuilder.AddSeparator();

	// Headings
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Heading1, LOCTEXT("H1", "H1"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Heading2, LOCTEXT("H2", "H2"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Heading3, LOCTEXT("H3", "H3"));

	ToolBarBuilder.AddSeparator();

	// Text formatting
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Bold, LOCTEXT("Bold", "B"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Italic, LOCTEXT("Italic", "I"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Strikethrough, LOCTEXT("Strikethrough", "S"));

	ToolBarBuilder.AddSeparator();

	// Insert elements
	AddCenteredToolBarButton(ToolBarBuilder, Commands.InsertCodeBlock, LOCTEXT("Code", "Code"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Blockquote, LOCTEXT("Quote", "Quote"));

	ToolBarBuilder.AddSeparator();

	// Lists and structure
	AddCenteredToolBarButton(ToolBarBuilder, Commands.BulletList, LOCTEXT("BulletList", "List"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.NumberedList, LOCTEXT("NumberedList", "1."));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.InsertTable, LOCTEXT("Table", "Table"));
	AddCenteredToolBarButton(ToolBarBuilder, Commands.HorizontalRule, LOCTEXT("HR", "HR"));

	ToolBarBuilder.AddSeparator();

	// Images
	AddCenteredToolBarButton(ToolBarBuilder, Commands.PasteImage, LOCTEXT("PasteImage", "Img"));

	ToolBarBuilder.AddSeparator();

	// Search
	AddCenteredToolBarButton(ToolBarBuilder, Commands.Find, LOCTEXT("FindButton", "Find"));
}

// ---- Markdown Formatting Helpers ----

void FMarkdownAssetEditorToolkit::WrapSelectionWith(const FString& Prefix, const FString& Suffix)
{
	if (!EditableTextBox.IsValid() || !MarkdownAsset)
	{
		return;
	}

	FString SelectedText = EditableTextBox->GetSelectedText().ToString();

	if (SelectedText.IsEmpty())
	{
		// No selection: insert placeholder wrapped with formatting markers
		EditableTextBox->InsertTextAtCursor(Prefix + TEXT("text") + Suffix);
	}
	else
	{
		// Replace the selected text with the wrapped version
		// InsertTextAtCursor replaces the current selection when text is selected
		EditableTextBox->InsertTextAtCursor(Prefix + SelectedText + Suffix);
	}
}

void FMarkdownAssetEditorToolkit::InsertAtLineStart(const FString& Prefix)
{
	if (!EditableTextBox.IsValid())
	{
		return;
	}

	FText CurrentText = EditableTextBox->GetText();
	FString TextStr = CurrentText.ToString();

	FTextLocation CursorLocation = EditableTextBox->GetCursorLocation();
	int32 LineIndex = CursorLocation.GetLineIndex();
	int32 Offset = CursorLocation.GetOffset();

	// Find the start of the line at LineIndex by counting \n characters
	int32 CurrentLine = 0;
	int32 LineStartPos = 0;

	for (int32 i = 0; i < TextStr.Len() && CurrentLine < LineIndex; ++i)
	{
		if (TextStr[i] == TEXT('\n'))
		{
			++CurrentLine;
			LineStartPos = i + 1;
		}
	}

	// Compute approximate absolute cursor position, clamped to text length
	int32 CursorAbsPos = FMath::Min(LineStartPos + Offset, TextStr.Len());

	// Search backward from cursor to find the true logical line start
	int32 LogicalLineStart = 0;
	for (int32 i = CursorAbsPos - 1; i >= 0; --i)
	{
		if (TextStr[i] == TEXT('\n'))
		{
			LogicalLineStart = i + 1;
			break;
		}
	}

	// Insert prefix at the logical line start
	TextStr.InsertAt(LogicalLineStart, Prefix);

	EditableTextBox->SetText(FText::FromString(TextStr));
	EditableTextBox->GoTo(FTextLocation(LineIndex, Offset + Prefix.Len()));
	ApplyTextToAsset(TextStr);
}

void FMarkdownAssetEditorToolkit::InsertTextAtCursor(const FString& Text)
{
	if (!EditableTextBox.IsValid())
	{
		return;
	}

	EditableTextBox->InsertTextAtCursor(Text);
}

// ---- Command Handlers ----

void FMarkdownAssetEditorToolkit::OnBold()
{
	WrapSelectionWith(TEXT("**"), TEXT("**"));
}

void FMarkdownAssetEditorToolkit::OnItalic()
{
	WrapSelectionWith(TEXT("*"), TEXT("*"));
}

void FMarkdownAssetEditorToolkit::OnStrikethrough()
{
	WrapSelectionWith(TEXT("~~"), TEXT("~~"));
}

void FMarkdownAssetEditorToolkit::OnInsertCodeBlock()
{
	InsertTextAtCursor(TEXT("\n```\ncode\n```\n"));
}

void FMarkdownAssetEditorToolkit::OnHeading1()
{
	InsertAtLineStart(TEXT("# "));
}

void FMarkdownAssetEditorToolkit::OnHeading2()
{
	InsertAtLineStart(TEXT("## "));
}

void FMarkdownAssetEditorToolkit::OnHeading3()
{
	InsertAtLineStart(TEXT("### "));
}

void FMarkdownAssetEditorToolkit::OnBulletList()
{
	InsertAtLineStart(TEXT("- "));
}

void FMarkdownAssetEditorToolkit::OnNumberedList()
{
	InsertAtLineStart(TEXT("1. "));
}

void FMarkdownAssetEditorToolkit::OnInsertTable()
{
	InsertTextAtCursor(TEXT("\n| Header 1 | Header 2 | Header 3 |\n| --- | --- | --- |\n| Cell 1 | Cell 2 | Cell 3 |\n"));
}

void FMarkdownAssetEditorToolkit::OnHorizontalRule()
{
	InsertTextAtCursor(TEXT("\n---\n"));
}

void FMarkdownAssetEditorToolkit::OnBlockquote()
{
	InsertAtLineStart(TEXT("> "));
}

void FMarkdownAssetEditorToolkit::OnPasteImage()
{
	TArray<uint8> Bgra;
	int32 Width = 0;
	int32 Height = 0;
	if (!MarkdownImageUtils::ReadClipboardImage(Bgra, Width, Height))
	{
		FNotificationInfo Info(LOCTEXT("PasteImageNoImage", "No image found in the clipboard."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	FString PackagePath;
	UTexture2D* Texture = MarkdownImageUtils::CreateTextureAssetFromBgra(Bgra, Width, Height, PackagePath);
	if (!Texture)
	{
		FNotificationInfo Info(LOCTEXT("PasteImageCreateFailed", "Failed to create a texture asset from the clipboard image."));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	// Inserting through the text box fires OnTextChanged, which updates the
	// asset and schedules the preview refresh that embeds the new texture.
	InsertTextAtCursor(FString::Printf(TEXT("![%s](%s)"), *Texture->GetName(), *PackagePath));

	FNotificationInfo Info(FText::Format(
		LOCTEXT("PasteImageCreated", "Created texture asset {0} and inserted an image link."),
		FText::FromString(PackagePath)));
	Info.ExpireDuration = 4.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void FMarkdownAssetEditorToolkit::OnAssetsDropped(const TArray<FAssetData>& DroppedAssets)
{
	if (!EditableTextBox.IsValid())
	{
		return;
	}

	FString Snippet;
	for (const FAssetData& Asset : DroppedAssets)
	{
		if (!Asset.IsValid())
		{
			continue;
		}

		if (!Snippet.IsEmpty())
		{
			Snippet += LINE_TERMINATOR;
		}

		// Resolving the class only loads the UClass (not the asset) for native types,
		// which is all BuildMarkdownLinkForAsset needs to pick the link syntax.
		Snippet += MarkdownLinkUtils::BuildMarkdownLinkForAsset(
			Asset.PackageName.ToString(), Asset.AssetName.ToString(), Asset.GetClass());
	}

	if (Snippet.IsEmpty())
	{
		return;
	}

	InsertTextAtCursor(Snippet);
	FSlateApplication::Get().SetKeyboardFocus(EditableTextBox);
}

// ---- Find & Replace ----

TSharedRef<SWidget> FMarkdownAssetEditorToolkit::BuildSearchBar()
{
	const FMargin ButtonPadding(2.0f, 0.0f, 0.0f, 0.0f);

	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(4.0f))
		.Visibility_Lambda([this]() { return bSearchBarVisible ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SVerticalBox)

			// Find row
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(FindTextBox, SEditableTextBox)
					.HintText(LOCTEXT("FindHint", "Find"))
					.OnTextChanged(this, &FMarkdownAssetEditorToolkit::OnFindTextChanged)
					.OnKeyDownHandler(FOnKeyDown::CreateSP(this, &FMarkdownAssetEditorToolkit::OnSearchBoxKeyDown, false))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(6.0f, 0.0f))
				[
					SNew(SBox)
					.MinDesiredWidth(70.0f)
					[
						SNew(STextBlock)
						.Text(this, &FMarkdownAssetEditorToolkit::GetSearchStatusText)
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(ButtonPadding)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.ToolTipText(LOCTEXT("MatchCaseTooltip", "Match case"))
					.IsChecked_Lambda([this]() { return bSearchMatchCase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						bSearchMatchCase = (State == ECheckBoxState::Checked);
						RefreshSearchMatches(true);
					})
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MatchCaseLabel", "Aa"))
						.Margin(FMargin(4.0f, 0.0f))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ButtonPadding)
				[
					SNew(SButton)
					.Text(LOCTEXT("FindPreviousButton", "Prev"))
					.ToolTipText(LOCTEXT("FindPreviousTooltip", "Select the previous match (Shift+F3)"))
					.OnClicked_Lambda([this]() { OnFindPrevious(); return FReply::Handled(); })
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ButtonPadding)
				[
					SNew(SButton)
					.Text(LOCTEXT("FindNextButton", "Next"))
					.ToolTipText(LOCTEXT("FindNextTooltip", "Select the next match (F3)"))
					.OnClicked_Lambda([this]() { OnFindNext(); return FReply::Handled(); })
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(ButtonPadding)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.ToolTipText(LOCTEXT("ToggleReplaceTooltip", "Show the Replace row (Ctrl+H)"))
					.IsChecked_Lambda([this]() { return bReplaceRowVisible ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
					{
						bReplaceRowVisible = (State == ECheckBoxState::Checked);
					})
					[
						SNew(STextBlock)
						.Text(LOCTEXT("ToggleReplaceLabel", "Replace"))
						.Margin(FMargin(4.0f, 0.0f))
					]
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ButtonPadding)
				[
					SNew(SButton)
					.Text(LOCTEXT("CloseSearchButton", "X"))
					.ToolTipText(LOCTEXT("CloseSearchTooltip", "Close (Esc)"))
					.OnClicked_Lambda([this]() { HideSearchBar(); return FReply::Handled(); })
				]
			]

			// Replace row
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				.Visibility_Lambda([this]() { return bReplaceRowVisible ? EVisibility::Visible : EVisibility::Collapsed; })

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(ReplaceTextBox, SEditableTextBox)
					.HintText(LOCTEXT("ReplaceHint", "Replace with"))
					.OnKeyDownHandler(FOnKeyDown::CreateSP(this, &FMarkdownAssetEditorToolkit::OnSearchBoxKeyDown, true))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ButtonPadding)
				[
					SNew(SButton)
					.Text(LOCTEXT("ReplaceButton", "Replace"))
					.ToolTipText(LOCTEXT("ReplaceTooltip", "Replace the selected match and move to the next one"))
					.OnClicked_Lambda([this]() { ReplaceCurrentMatch(); return FReply::Handled(); })
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(ButtonPadding)
				[
					SNew(SButton)
					.Text(LOCTEXT("ReplaceAllButton", "Replace All"))
					.ToolTipText(LOCTEXT("ReplaceAllTooltip", "Replace every match in the document"))
					.OnClicked_Lambda([this]() { ReplaceAllMatches(); return FReply::Handled(); })
				]
			]
		];
}

void FMarkdownAssetEditorToolkit::ShowSearchBar(bool bWithReplace)
{
	bSearchBarVisible = true;
	bReplaceRowVisible = bReplaceRowVisible || bWithReplace;

	// Seed the query from a single-line selection in the editor.
	if (EditableTextBox.IsValid() && FindTextBox.IsValid())
	{
		const FString Selected = EditableTextBox->GetSelectedText().ToString();
		if (!Selected.IsEmpty() && !Selected.Contains(TEXT("\n")) && !Selected.Contains(TEXT("\r")))
		{
			FindTextBox->SetText(FText::FromString(Selected));
		}
	}

	if (FindTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(FindTextBox);
		FindTextBox->SelectAllText();
	}

	RefreshSearchMatches(true);
}

void FMarkdownAssetEditorToolkit::HideSearchBar()
{
	bSearchBarVisible = false;

	if (EditableTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(EditableTextBox);
	}
}

void FMarkdownAssetEditorToolkit::OnFind()
{
	ShowSearchBar(false);
}

void FMarkdownAssetEditorToolkit::OnReplace()
{
	ShowSearchBar(true);
}

void FMarkdownAssetEditorToolkit::OnFindNext()
{
	if (!bSearchBarVisible)
	{
		ShowSearchBar(false);
		return;
	}
	SelectAdjacentMatch(true);
}

void FMarkdownAssetEditorToolkit::OnFindPrevious()
{
	if (!bSearchBarVisible)
	{
		ShowSearchBar(false);
		return;
	}
	SelectAdjacentMatch(false);
}

void FMarkdownAssetEditorToolkit::OnFindTextChanged(const FText& NewText)
{
	RefreshSearchMatches(true);
}

FReply FMarkdownAssetEditorToolkit::OnSearchBoxKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent, bool bIsReplaceBox)
{
	const FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::Escape)
	{
		HideSearchBar();
		return FReply::Handled();
	}

	if (Key == EKeys::Enter)
	{
		if (bIsReplaceBox)
		{
			ReplaceCurrentMatch();
		}
		else
		{
			SelectAdjacentMatch(!KeyEvent.IsShiftDown());
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FString FMarkdownAssetEditorToolkit::GetEditorText() const
{
	return EditableTextBox.IsValid() ? EditableTextBox->GetText().ToString() : FString();
}

void FMarkdownAssetEditorToolkit::RefreshSearchMatches(bool bSelectNearest)
{
	const FString Query = FindTextBox.IsValid() ? FindTextBox->GetText().ToString() : FString();
	const FString Text = GetEditorText();

	SearchMatches = MarkdownTextSearch::FindAll(Text, Query, bSearchMatchCase);
	CurrentMatchIndex = INDEX_NONE;

	if (!bSelectNearest || SearchMatches.Num() == 0 || !EditableTextBox.IsValid())
	{
		return;
	}

	// Keep the selection anchored where the user is: start from the beginning of
	// the current selection so that extending the query keeps the same match.
	const int32 FromOffset = MarkdownTextSearch::TextLocationToOffset(Text, EditableTextBox->GetSelection().GetBeginning());
	SelectMatch(MarkdownTextSearch::FindNextMatchIndex(SearchMatches, FromOffset, true));
}

void FMarkdownAssetEditorToolkit::SelectAdjacentMatch(bool bForward)
{
	if (!EditableTextBox.IsValid())
	{
		return;
	}

	// The text may have changed since the last search (typing in the editor).
	RefreshSearchMatches(false);
	if (SearchMatches.Num() == 0)
	{
		return;
	}

	const FString Text = GetEditorText();
	const FTextSelection Selection = EditableTextBox->GetSelection();
	const int32 FromOffset = bForward
		? MarkdownTextSearch::TextLocationToOffset(Text, Selection.GetEnd())
		: MarkdownTextSearch::TextLocationToOffset(Text, Selection.GetBeginning());

	SelectMatch(MarkdownTextSearch::FindNextMatchIndex(SearchMatches, FromOffset, bForward));
}

void FMarkdownAssetEditorToolkit::SelectMatch(int32 MatchIndex)
{
	if (!EditableTextBox.IsValid() || !SearchMatches.IsValidIndex(MatchIndex))
	{
		CurrentMatchIndex = INDEX_NONE;
		return;
	}

	const FString Text = GetEditorText();
	const FMarkdownTextMatch& Match = SearchMatches[MatchIndex];
	const FTextLocation Start = MarkdownTextSearch::OffsetToTextLocation(Text, Match.Start);
	const FTextLocation End = MarkdownTextSearch::OffsetToTextLocation(Text, Match.End());

	EditableTextBox->SelectText(Start, End);
	EditableTextBox->ScrollTo(Start);
	CurrentMatchIndex = MatchIndex;
}

void FMarkdownAssetEditorToolkit::ReplaceCurrentMatch()
{
	if (!EditableTextBox.IsValid() || !FindTextBox.IsValid())
	{
		return;
	}

	const FString Query = FindTextBox->GetText().ToString();
	if (Query.IsEmpty())
	{
		return;
	}

	const FString Replacement = ReplaceTextBox.IsValid() ? ReplaceTextBox->GetText().ToString() : FString();
	const FString Selected = EditableTextBox->GetSelectedText().ToString();
	const bool bSelectionIsMatch = bSearchMatchCase ? Selected.Equals(Query, ESearchCase::CaseSensitive) : Selected.Equals(Query, ESearchCase::IgnoreCase);

	if (bSelectionIsMatch)
	{
		// InsertTextAtCursor replaces the selection and fires OnTextChanged,
		// which syncs the asset and schedules the preview refresh.
		EditableTextBox->InsertTextAtCursor(Replacement);
	}

	SelectAdjacentMatch(true);
}

void FMarkdownAssetEditorToolkit::ReplaceAllMatches()
{
	if (!EditableTextBox.IsValid() || !FindTextBox.IsValid())
	{
		return;
	}

	const FString Query = FindTextBox->GetText().ToString();
	if (Query.IsEmpty())
	{
		return;
	}

	const FString Replacement = ReplaceTextBox.IsValid() ? ReplaceTextBox->GetText().ToString() : FString();

	int32 ReplacedCount = 0;
	const FString NewText = MarkdownTextSearch::ReplaceAll(GetEditorText(), Query, Replacement, bSearchMatchCase, ReplacedCount);
	if (ReplacedCount == 0)
	{
		return;
	}

	EditableTextBox->SetText(FText::FromString(NewText));
	ApplyTextToAsset(NewText);
	RefreshSearchMatches(false);

	FNotificationInfo Info(FText::Format(LOCTEXT("ReplaceAllDone", "Replaced {0} occurrence(s)."), ReplacedCount));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

FText FMarkdownAssetEditorToolkit::GetSearchStatusText() const
{
	const FString Query = FindTextBox.IsValid() ? FindTextBox->GetText().ToString() : FString();
	if (Query.IsEmpty())
	{
		return FText::GetEmpty();
	}

	if (SearchMatches.Num() == 0)
	{
		return LOCTEXT("SearchNoResults", "No results");
	}

	if (CurrentMatchIndex == INDEX_NONE)
	{
		return FText::Format(LOCTEXT("SearchMatchCount", "{0} match(es)"), SearchMatches.Num());
	}

	return FText::Format(LOCTEXT("SearchMatchPosition", "{0} of {1}"), CurrentMatchIndex + 1, SearchMatches.Num());
}

// ---- Tab Spawning ----

TSharedRef<SDockTab> FMarkdownAssetEditorToolkit::SpawnTab_Main(const FSpawnTabArgs& Args)
{
	FText InitialText = MarkdownAsset ? FText::FromString(MarkdownAsset->RawMarkdownText) : FText::GetEmpty();

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("EditorTabLabel", "Markdown Editor"))
		.TabRole(ETabRole::DocumentTab)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			// Left Panel: Find bar + raw Markdown editor (accepts Content Browser drops)
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					BuildSearchBar()
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SMarkdownAssetDropTarget)
					.OnAssetsDropped(FOnMarkdownAssetsDropped::CreateSP(this, &FMarkdownAssetEditorToolkit::OnAssetsDropped))
					[
						SAssignNew(EditableTextBox, SMultiLineEditableTextBox)
						.Text(InitialText)
						.OnTextChanged(this, &FMarkdownAssetEditorToolkit::OnTextChanged)
						.Font(GetEditorFont())
					]
				]
			]

			// Right Panel: HTML Preview
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(WebBrowserWidget, SWebBrowser)
				.ShowControls(false)
				.ShowAddressBar(false)
				.OnBeforeNavigation(this, &FMarkdownAssetEditorToolkit::HandleBeforeNavigation)
			]
		];

	// Force initial update to populate the browser
	if (WebBrowserWidget.IsValid() && MarkdownAsset)
	{
		UpdatePreview();
	}

	return SpawnedTab;
}

// ---- Outline Panel ----

TSharedRef<SDockTab> FMarkdownAssetEditorToolkit::SpawnTab_Outline(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("OutlineTabLabel", "Outline"))
		[
			SAssignNew(OutlineListView, SListView<TSharedPtr<FMarkdownHeading>>)
			.ListItemsSource(&OutlineItems)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &FMarkdownAssetEditorToolkit::OnGenerateOutlineRow)
			.OnMouseButtonClick(this, &FMarkdownAssetEditorToolkit::OnOutlineItemClicked)
		];

	RebuildOutline();

	return SpawnedTab;
}

void FMarkdownAssetEditorToolkit::RebuildOutline()
{
	OutlineItems.Reset();

	if (MarkdownAsset)
	{
		for (const FMarkdownHeading& Heading : MarkdownOutline::ExtractHeadings(MarkdownAsset->RawMarkdownText))
		{
			OutlineItems.Add(MakeShared<FMarkdownHeading>(Heading));
		}
	}

	if (OutlineListView.IsValid())
	{
		OutlineListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FMarkdownAssetEditorToolkit::OnGenerateOutlineRow(TSharedPtr<FMarkdownHeading> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const int32 Level = Item.IsValid() ? Item->Level : 1;
	const FString Label = (Item.IsValid() && !Item->Text.IsEmpty())
		? Item->Text
		: LOCTEXT("OutlineUntitledHeading", "(untitled)").ToString();

	return SNew(STableRow<TSharedPtr<FMarkdownHeading>>, OwnerTable)
		[
			SNew(SBox)
			.Padding(FMargin(4.0f + (Level - 1) * 12.0f, 2.0f, 4.0f, 2.0f))
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
				.ToolTipText(Item.IsValid()
					? FText::Format(LOCTEXT("OutlineItemTooltip", "H{0} - Line {1}"), Item->Level, Item->LineIndex + 1)
					: FText::GetEmpty())
				.Font(FCoreStyle::GetDefaultFontStyle(Level <= 2 ? "Bold" : "Regular", 9))
			]
		];
}

void FMarkdownAssetEditorToolkit::OnOutlineItemClicked(TSharedPtr<FMarkdownHeading> Item)
{
	if (!Item.IsValid())
	{
		return;
	}

	// Jump the text editor to the heading's line. The outline can lag the text by
	// the preview debounce, so clamp the line index to the current line count.
	if (EditableTextBox.IsValid())
	{
		const FString CurrentText = EditableTextBox->GetText().ToString();
		int32 LineCount = 1;
		for (const TCHAR Char : CurrentText)
		{
			if (Char == TEXT('\n'))
			{
				++LineCount;
			}
		}

		const FTextLocation Location(FMath::Clamp(Item->LineIndex, 0, LineCount - 1), 0);
		EditableTextBox->GoTo(Location);
		EditableTextBox->ScrollTo(Location);
		FSlateApplication::Get().SetKeyboardFocus(EditableTextBox);
	}

	// Scroll the HTML preview to the matching heading anchor.
	if (WebBrowserWidget.IsValid())
	{
		WebBrowserWidget->ExecuteJavascript(FString::Printf(
			TEXT("(function(){var e=document.getElementById('md-h-%d');if(e){e.scrollIntoView({behavior:'smooth',block:'start'});}})();"),
			Item->HeadingIndex));
	}
}

// ---- Wikilink Navigation ----

bool FMarkdownAssetEditorToolkit::HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request)
{
	using namespace MarkdownPreviewUtils;

	FString Payload;
	switch (ClassifyPreviewUrl(Url, &Payload))
	{
	// data: URLs for preview loading, and about:blank used by CEF during init.
	case EPreviewUrlAction::AllowInPage:
		return false;

	// mdasset:// scheme for wikilinks
	case EPreviewUrlAction::OpenMarkdownAsset:
		OpenLinkedMarkdownAsset(Payload);
		return true;

	// ueasset:// scheme for Content Browser asset / Blueprint object paths
	case EPreviewUrlAction::OpenUnrealAsset:
		OpenLinkedUnrealAsset(Payload);
		return true;

	// class:// scheme for C++ or Blueprint class references
	case EPreviewUrlAction::OpenClass:
		OpenLinkedClass(Payload);
		return true;

	// Open external URLs in the system browser after user confirmation.
	// The preview has no address bar, so we always prompt with the full URL
	// to let the user inspect it before launching (phishing mitigation).
	case EPreviewUrlAction::PromptExternal:
	{
		const FText Prompt = LOCTEXT("ConfirmExternalUrlMessage", "Open this URL in your default browser?");
		const FText Message = FText::FromString(
			FString::Printf(TEXT("%s\n\n%s"), *Prompt.ToString(), *Url)
		);
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			Message,
			LOCTEXT("ConfirmExternalUrlTitle", "Open External URL")
		);
		if (Response == EAppReturnType::Yes)
		{
			FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
		}
		return true;
	}

	// Default-deny: block unknown schemes (javascript:, file:, vbscript:, etc.)
	// to prevent script execution or local-file access from untrusted Markdown.
	case EPreviewUrlAction::Block:
	default:
		UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Blocked navigation to unsupported URL: '%s'"), *Url);
		return true;
	}
}

void FMarkdownAssetEditorToolkit::OpenLinkedMarkdownAsset(const FString& AssetName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> FoundAssets;
	AssetRegistry.GetAssetsByClass(UMarkdownAsset::StaticClass()->GetClassPathName(), FoundAssets);

	const FAssetData* MatchedAsset = FoundAssets.FindByPredicate(
		[&AssetName](const FAssetData& Asset)
		{
			return Asset.AssetName.ToString() == AssetName;
		});

	if (MatchedAsset)
	{
		if (UObject* LoadedAsset = MatchedAsset->GetAsset())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
		}
	}
	else
	{
		UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Wikilink target not found: '%s'"), *AssetName);
	}
}

void FMarkdownAssetEditorToolkit::OpenLinkedUnrealAsset(const FString& ObjectPath)
{
	if (ObjectPath.IsEmpty())
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Derive the package path (strip any trailing ".ObjectName") and the full object path.
	FString PackagePath = ObjectPath;
	int32 DotIndex;
	if (PackagePath.FindChar(TEXT('.'), DotIndex))
	{
		PackagePath.LeftInline(DotIndex);
	}

	int32 SlashIndex;
	FString LeafName;
	if (PackagePath.FindLastChar(TEXT('/'), SlashIndex))
	{
		LeafName = PackagePath.Mid(SlashIndex + 1);
	}
	const FString FullObjectPath = LeafName.IsEmpty() ? ObjectPath : FString::Printf(TEXT("%s.%s"), *PackagePath, *LeafName);

	UObject* LoadedAsset = nullptr;

	// Package-name query hits the registry's primary index and is the most reliable
	// path for standard Content Browser assets.
	TArray<FAssetData> PackageAssets;
	AssetRegistry.GetAssetsByPackageName(FName(*PackagePath), PackageAssets);
	if (PackageAssets.Num() > 0)
	{
		LoadedAsset = PackageAssets[0].GetAsset();
	}

	// GetAsset() has been observed to return nullptr on some UE5 builds despite
	// IsValid() reporting true. Fall back to loading the canonical path directly.
	if (!LoadedAsset)
	{
		for (const FString& Candidate : { ObjectPath, FullObjectPath })
		{
			FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(Candidate));
			if (AssetData.IsValid())
			{
				LoadedAsset = AssetData.GetSoftObjectPath().TryLoad();
				if (LoadedAsset) break;
			}
		}
	}

	// StaticLoadObject picks up packages that have not been indexed yet.
	if (!LoadedAsset)
	{
		LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *FullObjectPath);
	}

	// Last-resort package load so we can locate the first asset even when the
	// object name inside the package differs from the package leaf.
	if (!LoadedAsset && FPackageName::DoesPackageExist(PackagePath))
	{
		if (UPackage* LoadedPackage = LoadPackage(nullptr, *PackagePath, LOAD_None))
		{
			ForEachObjectWithPackage(LoadedPackage, [&LoadedAsset](UObject* Obj)
			{
				if (Obj && Obj->IsAsset()
#if UE_VERSION_OLDER_THAN(5, 6, 0)
					&& !Obj->IsA<UMetaData>()
#endif
					)
				{
					LoadedAsset = Obj;
					return false;
				}
				return true;
			}, false);
		}
	}

	if (!LoadedAsset)
	{
		UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Asset link target not found: '%s'"), *ObjectPath);
		return;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
	{
		AssetEditorSubsystem->OpenEditorForAsset(LoadedAsset);
	}
}

void FMarkdownAssetEditorToolkit::OpenLinkedClass(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return;
	}

	// Native UClass lookup against reflection names (UHT strips A/U/I prefixes).
	for (const FString& Candidate : MarkdownLinkUtils::BuildClassNameCandidates(ClassName))
	{
		UClass* FoundClass = FindFirstObject<UClass>(*Candidate, EFindFirstObjectOptions::NativeFirst);
		if (FoundClass && FoundClass->HasAnyClassFlags(CLASS_Native))
		{
			// Prefer the implementation file (.cpp); fall back to the header when no
			// .cpp is available (header-only classes, interfaces, etc.).
			FString SourcePath;
			if (FSourceCodeNavigation::FindClassSourcePath(FoundClass, SourcePath) && !SourcePath.IsEmpty())
			{
				if (FSourceCodeNavigation::OpenSourceFile(SourcePath))
				{
					return;
				}
			}

			if (!FSourceCodeNavigation::NavigateToClass(FoundClass))
			{
				UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Failed to open source for native class '%s'"), *FoundClass->GetName());
			}
			return;
		}
	}

	// Blueprint class fallback: accept both "BP_Foo" and "BP_Foo_C".
	FString BlueprintAssetName = ClassName;
	if (BlueprintAssetName.EndsWith(TEXT("_C")))
	{
		BlueprintAssetName.LeftChopInline(2);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);

	const FAssetData* MatchedBlueprint = BlueprintAssets.FindByPredicate(
		[&BlueprintAssetName](const FAssetData& Asset)
		{
			return Asset.AssetName.ToString() == BlueprintAssetName;
		});

	if (MatchedBlueprint)
	{
		if (UObject* LoadedBlueprint = MatchedBlueprint->GetAsset())
		{
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
			{
				AssetEditorSubsystem->OpenEditorForAsset(LoadedBlueprint);
			}
			return;
		}
	}

	UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Class link target not found: '%s'"), *ClassName);
}

#undef LOCTEXT_NAMESPACE
