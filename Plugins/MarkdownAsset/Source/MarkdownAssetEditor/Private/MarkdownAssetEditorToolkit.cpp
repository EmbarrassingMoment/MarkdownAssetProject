// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetEditorToolkit.h"
#include "MarkdownAsset.h"
#include "MarkdownOutline.h"
#include "MarkdownPreviewUtils.h"
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

/**
 * Builds reflection-name candidates to try when resolving a user-supplied class name.
 * UHT strips the leading A/U/I prefix from native class names, so "class://AActor"
 * must be matched against the UClass named "Actor". Conversely, a bare "Actor" may
 * need to be resolved as "AActor" for a hypothetical class in some projects.
 */
static TArray<FString> BuildClassNameCandidates(const FString& ClassName)
{
	TArray<FString> Candidates;
	if (ClassName.IsEmpty())
	{
		return Candidates;
	}

	Candidates.Add(ClassName);

	// Strip a single-letter A/U/I prefix when followed by an uppercase letter.
	if (ClassName.Len() > 1 && FChar::IsUpper(ClassName[1]))
	{
		const TCHAR First = ClassName[0];
		if (First == TEXT('A') || First == TEXT('U') || First == TEXT('I'))
		{
			Candidates.AddUnique(ClassName.Mid(1));
		}
	}

	// Add A/U-prefixed variants for bare names.
	if (FChar::IsUpper(ClassName[0]))
	{
		Candidates.AddUnique(FString::Printf(TEXT("A%s"), *ClassName));
		Candidates.AddUnique(FString::Printf(TEXT("U%s"), *ClassName));
	}

	return Candidates;
}

/** Returns true if an Unreal asset exists at the given object path. */
static bool DoesAssetExistAtPath(const FString& ObjectPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Strip a trailing ".ObjectName" to derive the package path; the registry indexes by package.
	FString PackagePath = ObjectPath;
	int32 DotIndex;
	if (PackagePath.FindChar(TEXT('.'), DotIndex))
	{
		PackagePath.LeftInline(DotIndex);
	}

	TArray<FAssetData> PackageAssets;
	AssetRegistry.GetAssetsByPackageName(FName(*PackagePath), PackageAssets);
	if (PackageAssets.Num() > 0)
	{
		return true;
	}

	// Secondary: legacy object-path query in case the caller supplied a non-package form.
	if (AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath)).IsValid())
	{
		return true;
	}

	// Last resort: does the package file exist on disk? Catches assets the registry has not
	// yet indexed (e.g. newly added plugin content).
	return FPackageName::DoesPackageExist(PackagePath);
}

/** Returns true if a native UClass or Blueprint class matching ClassName can be resolved. */
static bool DoesClassExist(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return false;
	}

	// Native class lookup against reflection names (UHT strips A/U/I prefixes).
	for (const FString& Candidate : BuildClassNameCandidates(ClassName))
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
		const FString DecodedTarget = MarkdownPreviewUtils::PercentDecode(EncodedTarget);

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
const FName FMarkdownAssetEditorToolkit::OutlineTabId(TEXT("MarkdownAssetEditor_OutlineTab"));

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
	RebuildOutline();

	if (MarkdownAsset && WebBrowserWidget.IsValid())
	{
		FString ParsedHtml = MarkdownAsset->GetParsedHTML();
		ParsedHtml = MarkBrokenLinks(ParsedHtml);
		ParsedHtml = MarkdownOutline::InjectHeadingAnchors(ParsedHtml);

		FString StyledHtml = MarkdownPreviewUtils::GenerateStyledHtml(ParsedHtml);

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
	for (const FString& Candidate : BuildClassNameCandidates(ClassName))
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
