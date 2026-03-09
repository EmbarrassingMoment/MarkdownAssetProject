// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetExporter.h"
#include "MarkdownAsset.h"
#include "Misc/FeedbackContext.h"
#include "UObject/UObjectGlobals.h"

UMarkdownAssetExporter::UMarkdownAssetExporter()
{
	SupportedClass = UMarkdownAsset::StaticClass();
	FormatExtension.Add(TEXT("md"));
	FormatDescription.Add(TEXT("Markdown File"));
	bText = true;
	PreferredFormatIndex = 0;
}

bool UMarkdownAssetExporter::ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Object);
	if (MarkdownAsset)
	{
		Ar.Log(MarkdownAsset->RawMarkdownText);
		return true;
	}
	return false;
}
