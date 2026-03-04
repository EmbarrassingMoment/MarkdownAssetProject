#include "MarkdownAssetEditorToolkit.h"
#include "MarkdownAsset.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/SMultiLineEditableTextBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Styling/CoreStyle.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

const FName FMarkdownAssetEditorToolkit::AppIdentifier(TEXT("MarkdownAssetEditorApp"));
const FName FMarkdownAssetEditorToolkit::MainTabId(TEXT("MarkdownAssetEditor_MainTab"));

void FMarkdownAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = WorkspaceMenu::GetWorkspaceMenuStructure().GetLevelEditorCategory();

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

		if (RichTextBlock.IsValid())
		{
			RichTextBlock->SetText(FText::FromString(MarkdownAsset->GetParsedHTML()));
		}
	}
}

TSharedRef<SDockTab> FMarkdownAssetEditorToolkit::SpawnTab_Main(const FSpawnTabArgs& Args)
{
	FText InitialText = MarkdownAsset ? FText::FromString(MarkdownAsset->RawMarkdownText) : FText::GetEmpty();
	FText InitialHTML = MarkdownAsset ? FText::FromString(MarkdownAsset->GetParsedHTML()) : FText::GetEmpty();

	return SNew(SDockTab)
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
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(RichTextBlock, SRichTextBlock)
					.Text(InitialHTML)
				]
			]
		];
}
