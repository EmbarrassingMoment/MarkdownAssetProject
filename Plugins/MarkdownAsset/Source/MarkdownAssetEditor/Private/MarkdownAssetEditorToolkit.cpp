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
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Internationalization/Regex.h"
#include "SourceCodeNavigation.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SoftObjectPath.h"

#define LOCTEXT_NAMESPACE "MarkdownAssetEditor"

DEFINE_LOG_CATEGORY_STATIC(LogMarkdownAssetEditor, Log, All);

/** Decodes a percent-encoded URI string back to a regular FString. */
static FString PercentDecode(const FString& Input)
{
	TArray<uint8> Bytes;
	Bytes.Reserve(Input.Len());

	for (int32 i = 0; i < Input.Len(); ++i)
	{
		if (Input[i] == TEXT('%') && i + 2 < Input.Len())
		{
			FString HexStr = Input.Mid(i + 1, 2);
			uint8 Value = static_cast<uint8>(FCString::Strtoi(*HexStr, nullptr, 16));
			Bytes.Add(Value);
			i += 2;
		}
		else if (Input[i] == TEXT('+'))
		{
			Bytes.Add(static_cast<uint8>(' '));
		}
		else
		{
			// ASCII range character
			Bytes.Add(static_cast<uint8>(Input[i] & 0xFF));
		}
	}

	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	return FString(Converter.Length(), Converter.Get());
}

// ---- Styled HTML Generation ----

/**
 * Wraps the parsed HTML content in a full HTML document with dark-themed CSS styling.
 */
static FString GenerateStyledHtml(const FString& ParsedHtml)
{
	return FString::Printf(TEXT(
		"<!DOCTYPE html>\n"
		"<html><head>\n"
		"<meta charset=\"utf-8\">\n"
		"<style>\n"
		"body { font-family: 'Segoe UI', 'Meiryo', 'Yu Gothic', sans-serif; background-color: #1e1e1e; color: #cccccc; padding: 20px; }\n"
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
"a[href^=\"mdasset://\"] { color: #4ec9b0; text-decoration: none; border-bottom: 1px dashed #4ec9b0; cursor: pointer; }\n"
"a[href^=\"mdasset://\"]:hover { color: #6fe0c8; border-bottom-style: solid; }\n"
"a[href^=\"ueasset://\"] { color: #dcdcaa; text-decoration: none; border-bottom: 1px dashed #dcdcaa; cursor: pointer; }\n"
"a[href^=\"ueasset://\"]:hover { color: #f1e9a6; border-bottom-style: solid; }\n"
"a[href^=\"class://\"] { color: #c586c0; text-decoration: none; border-bottom: 1px dashed #c586c0; cursor: pointer; }\n"
"a[href^=\"class://\"]:hover { color: #d7a7d2; border-bottom-style: solid; }\n"
"a.md-broken-link { color: #f44747; border-bottom-color: #f44747; }\n"
"a.md-broken-link:hover { color: #ff6b6b; }\n"
		"</style></head><body>\n%s\n</body></html>"
	), *ParsedHtml);
}

/** Returns true if an Unreal asset exists at the given object path. */
static bool DoesAssetExistAtPath(const FString& ObjectPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	const FSoftObjectPath SoftPath(ObjectPath);
	if (AssetRegistry.GetAssetByObjectPath(SoftPath).IsValid())
	{
		return true;
	}

	// Content Browser paths often omit the trailing ".AssetName"; try appending it.
	int32 SlashIndex;
	if (!ObjectPath.Contains(TEXT(".")) && ObjectPath.FindLastChar(TEXT('/'), SlashIndex))
	{
		const FString LeafName = ObjectPath.Mid(SlashIndex + 1);
		const FString FullPath = FString::Printf(TEXT("%s.%s"), *ObjectPath, *LeafName);
		if (AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(FullPath)).IsValid())
		{
			return true;
		}
	}

	return false;
}

/** Returns true if a native UClass or Blueprint class matching ClassName can be resolved. */
static bool DoesClassExist(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return false;
	}

	// Native class lookup (tries exact name, then common A/U prefixes).
	TArray<FString> Candidates = { ClassName };
	if (!ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
	{
		Candidates.Add(FString::Printf(TEXT("A%s"), *ClassName));
		Candidates.Add(FString::Printf(TEXT("U%s"), *ClassName));
	}
	for (const FString& Candidate : Candidates)
	{
		if (FindFirstObject<UClass>(*Candidate, EFindFirstObjectOptions::NativeFirst) != nullptr)
		{
			return true;
		}
	}

	// Blueprint fallback via Asset Registry (accepts both "BP_Foo" and "BP_Foo_C").
	FString BlueprintAssetName = ClassName;
	if (BlueprintAssetName.EndsWith(TEXT("_C")))
	{
		BlueprintAssetName.LeftChopInline(2);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
	for (const FAssetData& Asset : BlueprintAssets)
	{
		if (Asset.AssetName.ToString() == BlueprintAssetName)
		{
			return true;
		}
	}

	return false;
}

/**
 * Walks every <a href="..."> tag and adds the "md-broken-link" CSS class when
 * the target of an mdasset://, ueasset://, or class:// scheme cannot be resolved.
 * External URLs and other schemes are left untouched.
 */
static FString MarkBrokenLinks(const FString& Html)
{
	// Cache MarkdownAsset names once for mdasset:// lookups.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AllMarkdownAssets;
	AssetRegistry.GetAssetsByClass(UMarkdownAsset::StaticClass()->GetClassPathName(), AllMarkdownAssets);

	TSet<FString> MarkdownAssetNames;
	for (const FAssetData& Asset : AllMarkdownAssets)
	{
		MarkdownAssetNames.Add(Asset.AssetName.ToString());
	}

	const FRegexPattern LinkPattern(TEXT("<a href=\"(mdasset|ueasset|class)://([^\"]*)\""));
	FRegexMatcher Matcher(LinkPattern, Html);

	FString Result;
	int32 LastPos = 0;

	while (Matcher.FindNext())
	{
		Result += Html.Mid(LastPos, Matcher.GetMatchBeginning() - LastPos);

		const FString Scheme = Matcher.GetCaptureGroup(1);
		const FString EncodedTarget = Matcher.GetCaptureGroup(2);
		const FString DecodedTarget = PercentDecode(EncodedTarget);

		bool bTargetExists = false;
		if (Scheme == TEXT("mdasset"))
		{
			bTargetExists = MarkdownAssetNames.Contains(DecodedTarget);
		}
		else if (Scheme == TEXT("ueasset"))
		{
			bTargetExists = DoesAssetExistAtPath(DecodedTarget);
		}
		else // class
		{
			bTargetExists = DoesClassExist(DecodedTarget);
		}

		if (bTargetExists)
		{
			Result += Html.Mid(Matcher.GetMatchBeginning(), Matcher.GetMatchEnding() - Matcher.GetMatchBeginning());
		}
		else
		{
			Result += FString::Printf(TEXT("<a class=\"md-broken-link\" href=\"%s://%s\""), *Scheme, *EncodedTarget);
		}

		LastPos = Matcher.GetMatchEnding();
	}
	Result += Html.Mid(LastPos);

	return Result;
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
		ParsedHtml = MarkBrokenLinks(ParsedHtml);
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

// ---- Wikilink Navigation ----

bool FMarkdownAssetEditorToolkit::HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request)
{
	// Allow data: URLs for preview loading
	if (Url.StartsWith(TEXT("data:")))
	{
		return false;
	}

	// Handle mdasset:// scheme for wikilinks
	static const FString MdAssetScheme = TEXT("mdasset://");
	if (Url.StartsWith(MdAssetScheme))
	{
		FString AssetName = PercentDecode(Url.Mid(MdAssetScheme.Len()));
		OpenLinkedMarkdownAsset(AssetName);
		return true;
	}

	// Handle ueasset:// scheme for Content Browser asset / Blueprint object paths
	static const FString UEAssetScheme = TEXT("ueasset://");
	if (Url.StartsWith(UEAssetScheme))
	{
		FString ObjectPath = PercentDecode(Url.Mid(UEAssetScheme.Len()));
		OpenLinkedUnrealAsset(ObjectPath);
		return true;
	}

	// Handle class:// scheme for C++ or Blueprint class references
	static const FString ClassScheme = TEXT("class://");
	if (Url.StartsWith(ClassScheme))
	{
		FString ClassName = PercentDecode(Url.Mid(ClassScheme.Len()));
		OpenLinkedClass(ClassName);
		return true;
	}

	// Open external URLs in the system browser
	if (Url.StartsWith(TEXT("http://")) || Url.StartsWith(TEXT("https://")))
	{
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
		return true;
	}

	return false;
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

	// Try the path as provided, then the common "/Game/Foo/Bar.Bar" form.
	TArray<FString> CandidatePaths;
	CandidatePaths.Add(ObjectPath);

	int32 SlashIndex;
	if (!ObjectPath.Contains(TEXT(".")) && ObjectPath.FindLastChar(TEXT('/'), SlashIndex))
	{
		const FString LeafName = ObjectPath.Mid(SlashIndex + 1);
		CandidatePaths.Add(FString::Printf(TEXT("%s.%s"), *ObjectPath, *LeafName));
	}

	for (const FString& Candidate : CandidatePaths)
	{
		const FSoftObjectPath SoftPath(Candidate);
		FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(SoftPath);
		if (!AssetData.IsValid())
		{
			continue;
		}

		if (UObject* LoadedAsset = AssetData.GetAsset())
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedAsset);
			return;
		}
	}

	UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Asset link target not found: '%s'"), *ObjectPath);
}

void FMarkdownAssetEditorToolkit::OpenLinkedClass(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return;
	}

	// Native UClass lookup; try common UE prefixes if the bare name misses.
	TArray<FString> NativeCandidates = { ClassName };
	if (!ClassName.StartsWith(TEXT("A")) && !ClassName.StartsWith(TEXT("U")))
	{
		NativeCandidates.Add(FString::Printf(TEXT("A%s"), *ClassName));
		NativeCandidates.Add(FString::Printf(TEXT("U%s"), *ClassName));
	}

	for (const FString& Candidate : NativeCandidates)
	{
		if (UClass* FoundClass = FindFirstObject<UClass>(*Candidate, EFindFirstObjectOptions::NativeFirst))
		{
			if (FoundClass->HasAnyClassFlags(CLASS_Native))
			{
				if (!FSourceCodeNavigation::NavigateToClass(FoundClass))
				{
					UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Failed to open source for native class '%s'"), *FoundClass->GetName());
				}
				return;
			}
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
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LoadedBlueprint);
			return;
		}
	}

	UE_LOG(LogMarkdownAssetEditor, Warning, TEXT("Class link target not found: '%s'"), *ClassName);
}

#undef LOCTEXT_NAMESPACE
