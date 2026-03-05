#include "MarkdownAssetEditorToolkit.h"
#include "MarkdownAsset.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Styling/CoreStyle.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "SWebBrowser.h"
#include "Misc/Base64.h"

static FString GenerateStyledHtml(const FString& ParsedHtml)
{
	return FString::Printf(TEXT(
		"<!DOCTYPE html>\n"
		"<html><head><style>\n"
		"body { font-family: 'Segoe UI', sans-serif; background-color: #1e1e1e; color: #cccccc; padding: 20px; }\n"
		"h1, h2, h3, h4, h5, h6 { color: #ffffff; border-bottom: 1px solid #444; padding-bottom: 5px; }\n"
		"code { background-color: #2d2d2d; padding: 2px 4px; border-radius: 4px; }\n"
		"pre { background-color: #2d2d2d; padding: 10px; border-radius: 4px; overflow-x: auto; }\n"
		"a { color: #3794ff; }\n"
		"</style></head><body>\n%s\n</body></html>"
	), *ParsedHtml);
}

const FName FMarkdownAssetEditorToolkit::AppIdentifier(TEXT("MarkdownAssetEditorApp"));
const FName FMarkdownAssetEditorToolkit::MainTabId(TEXT("MarkdownAssetEditor_MainTab"));

void FMarkdownAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(MainTabId, FOnSpawnTab::CreateSP(this, &FMarkdownAssetEditorToolkit::SpawnTab_Main))
		.SetDisplayName(FText::FromString(TEXT("Markdown Editor")))
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
}

FName FMarkdownAssetEditorToolkit::GetToolkitFName() const
{
	return FName("MarkdownAssetEditor");
}

FText FMarkdownAssetEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString("Markdown Asset Editor");
}

FString FMarkdownAssetEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("MarkdownAsset");
}

FLinearColor FMarkdownAssetEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FMarkdownAssetEditorToolkit::OnTextChanged(const FText& NewText)
{
	if (MarkdownAsset)
	{
		MarkdownAsset->RawMarkdownText = NewText.ToString();
		MarkdownAsset->MarkPackageDirty();

		if (WebBrowserWidget.IsValid())
		{
			FString ParsedHtml = MarkdownAsset->GetParsedHTML();
			FString HtmlContent = GenerateStyledHtml(ParsedHtml);
			FString Base64Html = FBase64::Encode(HtmlContent);
			FString DataUrl = FString::Printf(TEXT("data:text/html;base64,%s"), *Base64Html);
			WebBrowserWidget->LoadURL(DataUrl);
		}
	}
}

TSharedRef<SDockTab> FMarkdownAssetEditorToolkit::SpawnTab_Main(const FSpawnTabArgs& Args)
{
	FText InitialText = MarkdownAsset ? FText::FromString(MarkdownAsset->RawMarkdownText) : FText::GetEmpty();

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(FText::FromString("Markdown Editor"))
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
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
			]

			// Right Panel: HTML Preview
			+ SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(WebBrowserWidget, SWebBrowser)
				.InitialURL(TEXT("about:blank"))
			]
		];

	if (WebBrowserWidget.IsValid() && MarkdownAsset)
	{
		FString InitialHtml = GenerateStyledHtml(MarkdownAsset->GetParsedHTML());
		FString Base64Html = FBase64::Encode(InitialHtml);
		FString DataUrl = FString::Printf(TEXT("data:text/html;base64,%s"), *Base64Html);
		WebBrowserWidget->LoadURL(DataUrl);
	}

	return SpawnedTab;
}
