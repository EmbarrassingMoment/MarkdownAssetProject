// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetFactory.h"
#include "MarkdownAsset.h"
#include "Misc/FileHelper.h"

UMarkdownAssetFactory::UMarkdownAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
	bText = true;
	SupportedClass = UMarkdownAsset::StaticClass();
	Formats.Add(TEXT("md;Markdown File"));
	Formats.Add(TEXT("markdown;Markdown File"));
}

UObject* UMarkdownAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
}

UObject* UMarkdownAssetFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	UMarkdownAsset* NewAsset = NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
	if (NewAsset)
	{
		// The engine has already handled the encoding (UTF-8, UTF-16, etc.) and converted it to TCHAR.
		NewAsset->RawMarkdownText = FString((int32)(BufferEnd - Buffer), Buffer);
	}
	return NewAsset;
}
