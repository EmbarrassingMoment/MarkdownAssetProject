// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetImportFactory.h"
#include "MarkdownAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

/** Configures the factory to handle .md and .markdown file imports as text. */
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

/** Creates a UMarkdownAsset from the text buffer and records the source file path for reimport. */
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
		NewAsset->SourceFilePath = GetCurrentFilename();
#endif
	}
	return NewAsset;
}

/** Returns true if the object is a UMarkdownAsset with a valid source file path. */
bool UMarkdownAssetImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
	if (MarkdownAsset)
	{
#if WITH_EDITORONLY_DATA
		if (!MarkdownAsset->SourceFilePath.IsEmpty())
		{
			OutFilenames.Add(MarkdownAsset->SourceFilePath);
			return true;
		}
#endif
	}
	return false;
}

/** Updates the source file path stored on the Markdown asset for future reimports. */
void UMarkdownAssetImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
	if (MarkdownAsset && ensure(NewReimportPaths.Num() == 1))
	{
#if WITH_EDITORONLY_DATA
		MarkdownAsset->SourceFilePath = NewReimportPaths[0];
#endif
	}
}

/** Reloads the Markdown content from the source file on disk and updates the asset. */
EReimportResult::Type UMarkdownAssetImportFactory::Reimport(UObject* Obj)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Obj);
	if (!MarkdownAsset)
	{
		return EReimportResult::Failed;
	}

#if WITH_EDITORONLY_DATA
	const FString Filename = MarkdownAsset->SourceFilePath;
	if (Filename.IsEmpty() || !FPaths::FileExists(Filename))
	{
		return EReimportResult::Failed;
	}

	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *Filename))
	{
		MarkdownAsset->RawMarkdownText = FileContent;
		MarkdownAsset->MarkPackageDirty();
		return EReimportResult::Succeeded;
	}
#endif

	return EReimportResult::Failed;
}

/** Returns the default import priority for this factory. */
int32 UMarkdownAssetImportFactory::GetPriority() const
{
	return ImportPriority;
}
