// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorToolkit.h"
#include "MarkdownAsset.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
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

#define LOCTEXT_NAMESPACE "MarkdownAssetEditor"

// ---- Styled HTML Generation ----

static FString GenerateStyledHtml(const FString& ParsedHtml)
{
	return FString::Printf(TEXT(
		"<!DOCTYPE html>\n"
		"<html><head>\n"
		"<meta charset=\"utf-8\">\n"
		"<style>\n"
		"body { font-family: 'Segoe UI', sans-serif; background-color: #1e1e1e; color: #cccccc; padding: 20px; }\n"
		"h1, h2, h3, h4, h5, h6 { color: #ffffff; border-bottom: 1px solid #444; padding-bottom: 5px; }\n"
		"code { background-color: #2d2d2d; padding: 2px 4px; border-radius: 4px; }\n"
		"pre { background-color: #2d2d2d; padding: 10px; border-radius: 4px; overflow-x: auto; }\n"
		"a { color: #3794ff; }\n"
		"table { border-collapse: collapse; width: 100%%; margin-bottom: 20px; }\n"
		"th, td { border: 1px solid #444; padding: 8px 12px; text-align: left; }\n"
		"th { background-color: #333; color: #fff; font-weight: bold; }\n"
		"tr:nth-child(even) { background-color: #2a2a2a; }\n"
		"blockquote { border-left: 4px solid #3794ff; margin: 10px 0; padding: 5px 15px; background-color: #252525; }\n"
		"hr { border: none; border-top: 1px solid #444; margin: 20px 0; }\n"
		"img { max-width: 100%%; height: auto; }\n"
		"del { color: #888; }\n"
		"</style></head><body>\n%s\n</body></html>"
	), *ParsedHtml);
}

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
}

// ---- FMarkdownAssetEditorToolkit ----

const FName FMarkdownAssetEditorToolkit::AppIdentifier(TEXT("MarkdownAssetEditorApp"));
const FName FMarkdownAssetEditorToolkit::MainTabId(TEXT("MarkdownAssetEditor_MainTab"));

FMarkdownAssetEditorToolkit::~FMarkdownAssetEditorToolkit()
{
	FMarkdownEditorCommands::Unregister();

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
}

void FMarkdownAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(MainTabId);
}

void FMarkdownAssetEditorToolkit::Initialize(UMarkdownAsset* InMarkdownAsset, const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost)
{
	MarkdownAsset = InMarkdownAsset;

	FMarkdownEditorCommands::Register();
	BindCommands();

	// Create the layout
	TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MarkdownAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(1.0f)
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
	if (MarkdownAsset)
	{
		{
			FScopedTransaction Transaction(LOCTEXT("EditMarkdownText", "Edit Markdown Text"));
			MarkdownAsset->Modify();
			MarkdownAsset->RawMarkdownText = NewText.ToString();
		}
		MarkdownAsset->MarkPackageDirty();

		if (GEditor)
		{
			GEditor->GetTimerManager()->ClearTimer(PreviewUpdateTimerHandle);
			GEditor->GetTimerManager()->SetTimer(
				PreviewUpdateTimerHandle,
				FTimerDelegate::CreateSP(this, &FMarkdownAssetEditorToolkit::UpdatePreview),
				0.3f,
				false
			);
		}
	}
}

void FMarkdownAssetEditorToolkit::UpdatePreview()
{
	if (MarkdownAsset && WebBrowserWidget.IsValid())
	{
		FString ParsedHtml = MarkdownAsset->GetParsedHTML();
		FString StyledHtml = GenerateStyledHtml(ParsedHtml);

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
}

// ---- Markdown Formatting Helpers ----

void FMarkdownAssetEditorToolkit::WrapSelectionWith(const FString& Prefix, const FString& Suffix)
{
	if (!EditableTextBox.IsValid() || !MarkdownAsset)
	{
		return;
	}

	FText CurrentText = EditableTextBox->GetText();
	FString TextStr = CurrentText.ToString();

	// SMultiLineEditableTextBox doesn't expose selection, so insert at end
	// This is a simplification - ideally we'd wrap selected text
	FString SelectedText = EditableTextBox->GetSelectedText().ToString();

	if (SelectedText.IsEmpty())
	{
		SelectedText = TEXT("text");
	}

	FString Replacement = Prefix + SelectedText + Suffix;

	// If there's selected text, try to replace it; otherwise append
	if (!EditableTextBox->GetSelectedText().IsEmpty())
	{
		EditableTextBox->InsertTextAtCursor(Replacement);
	}
	else
	{
		EditableTextBox->InsertTextAtCursor(Replacement);
	}
}

void FMarkdownAssetEditorToolkit::InsertAtLineStart(const FString& Prefix)
{
	if (!EditableTextBox.IsValid())
	{
		return;
	}

	EditableTextBox->InsertTextAtCursor(Prefix);
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

			// Left Panel: Raw Markdown Editor
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(EditableTextBox, SMultiLineEditableTextBox)
				.Text(InitialText)
				.OnTextChanged(this, &FMarkdownAssetEditorToolkit::OnTextChanged)
				.Font(FCoreStyle::GetDefaultFontStyle("Mono", 12))
			]

			// Right Panel: HTML Preview
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(WebBrowserWidget, SWebBrowser)
				.ShowControls(false)
				.ShowAddressBar(false)
			]
		];

	// Force initial update to populate the browser
	if (WebBrowserWidget.IsValid() && MarkdownAsset)
	{
		UpdatePreview();
	}

	return SpawnedTab;
}

#undef LOCTEXT_NAMESPACE
