// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetImportFactory.h"
#include "MarkdownAsset.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"

UMarkdownAssetImportFactory::UMarkdownAssetImportFactory()
{
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true;
	bText = true;
	SupportedClass = UMarkdownAsset::StaticClass();
	Formats.Add(TEXT("md;Markdown File"));
	Formats.Add(TEXT("markdown;Markdown File"));
}

UObject* UMarkdownAssetImportFactory::FactoryCreateText(
	UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context,
	const TCHAR* Type, const TCHAR*& Buffer,
	const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	UMarkdownAsset* NewAsset = NewObject<UMarkdownAsset>(
		InParent, InClass, InName, Flags | RF_Transactional);
	if (NewAsset)
	{
		NewAsset->RawMarkdownText = FString(
			(int32)(BufferEnd - Buffer), Buffer);

#if WITH_EDITORONLY_DATA
		if (!CurrentFilename.IsEmpty())
		{
			if (!NewAsset->AssetImportData)
			{
				NewAsset->AssetImportData = NewObject<UAssetImportData>(NewAsset);
			}
			NewAsset->AssetImportData->Update(CurrentFilename);
		}
#endif
	}
	return NewAsset;
}

bool UMarkdownAssetImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
#if WITH_EDITORONLY_DATA
	if (MarkdownAsset && MarkdownAsset->AssetImportData)
	{
		MarkdownAsset->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
#endif
	return false;
}

void UMarkdownAssetImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
#if WITH_EDITORONLY_DATA
	if (MarkdownAsset && MarkdownAsset->AssetImportData && NewReimportPaths.Num() > 0)
	{
		MarkdownAsset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
#endif
}

EReimportResult::Type UMarkdownAssetImportFactory::Reimport(UObject* Obj)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
	if (!MarkdownAsset)
	{
		return EReimportResult::Failed;
	}

#if WITH_EDITORONLY_DATA
	// ソースファイルパスを取得
	const FString SourceFilePath = MarkdownAsset->AssetImportData
		? MarkdownAsset->AssetImportData->GetFirstFilename()
		: FString();

	if (SourceFilePath.IsEmpty() || !FPaths::FileExists(SourceFilePath))
	{
		return EReimportResult::Failed;
	}

	// ファイルを読み込み
	FString FileContents;
	if (!FFileHelper::LoadFileToString(FileContents, *SourceFilePath))
	{
		return EReimportResult::Failed;
	}

	// アセットを更新
	MarkdownAsset->RawMarkdownText = FileContents;
	MarkdownAsset->AssetImportData->Update(SourceFilePath);
	MarkdownAsset->MarkPackageDirty();

	return EReimportResult::Succeeded;
#else
	return EReimportResult::Failed;
#endif
}
