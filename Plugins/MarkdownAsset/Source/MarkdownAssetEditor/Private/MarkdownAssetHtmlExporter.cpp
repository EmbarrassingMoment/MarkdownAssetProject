// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetHtmlExporter.h"
#include "MarkdownAsset.h"
#include "MarkdownImageUtils.h"
#include "MarkdownLinkUtils.h"
#include "MarkdownOutline.h"
#include "MarkdownPreviewUtils.h"

UMarkdownAssetHtmlExporter::UMarkdownAssetHtmlExporter()
{
	SupportedClass = UMarkdownAsset::StaticClass();
	FormatExtension.Add(TEXT("html"));
	FormatDescription.Add(TEXT("Standalone HTML File"));
	// Binary export so the document is written as UTF-8 regardless of the platform's
	// default text encoding; text exporters may fall back to UTF-16 for non-ASCII content.
	bText = false;
	PreferredFormatIndex = 0;
}

bool UMarkdownAssetHtmlExporter::ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags)
{
	const UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Object);
	if (!MarkdownAsset)
	{
		return false;
	}

	const UMarkdownEditorSettings* Settings = GetDefault<UMarkdownEditorSettings>();
	const EMarkdownPreviewTheme Theme = Settings ? Settings->GetExportTheme() : EMarkdownPreviewTheme::Dark;

	const FString Html = BuildStandaloneHtml(MarkdownAsset, Theme);

	FTCHARToUTF8 Utf8(*Html);
	Ar.Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
	return true;
}

FString UMarkdownAssetHtmlExporter::BuildStandaloneHtml(const UMarkdownAsset* MarkdownAsset, EMarkdownPreviewTheme Theme)
{
	if (!MarkdownAsset)
	{
		return FString();
	}

	FString ParsedHtml = MarkdownAsset->GetParsedHTML();
	ParsedHtml = MarkdownLinkUtils::MarkBrokenLinks(ParsedHtml);
	ParsedHtml = MarkdownOutline::InjectHeadingAnchors(ParsedHtml);
	ParsedHtml = MarkdownImageUtils::EmbedTextureImages(ParsedHtml);

	MarkdownPreviewUtils::FStyledHtmlOptions Options;
	Options.Theme = Theme;
	Options.ContentSecurityPolicy = MarkdownPreviewUtils::EContentSecurityPolicy::StandaloneExport;
	Options.Title = MarkdownAsset->GetName();

	return MarkdownPreviewUtils::GenerateStyledHtml(ParsedHtml, Options);
}
