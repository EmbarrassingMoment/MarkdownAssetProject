// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetImportFactory.h"
#include "MarkdownAsset.h"

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
	}
	return NewAsset;
}
