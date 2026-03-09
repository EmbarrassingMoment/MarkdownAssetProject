// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetFactory.h"
#include "MarkdownAsset.h"
#include "Misc/FileHelper.h"

UMarkdownAssetFactory::UMarkdownAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
	SupportedClass = UMarkdownAsset::StaticClass();
	Formats.Add(TEXT("md;Markdown File"));
	Formats.Add(TEXT("markdown;Markdown File"));
}

UObject* UMarkdownAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
}

UObject* UMarkdownAssetFactory::FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	UMarkdownAsset* NewAsset = NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags);
	if (NewAsset)
	{
		FString FileContent;
		// Safely convert the binary buffer to an FString (handles UTF-8 and BOMs)
		FFileHelper::BufferToString(FileContent, Buffer, BufferEnd - Buffer);
		NewAsset->RawMarkdownText = FileContent;
	}
	return NewAsset;
}
