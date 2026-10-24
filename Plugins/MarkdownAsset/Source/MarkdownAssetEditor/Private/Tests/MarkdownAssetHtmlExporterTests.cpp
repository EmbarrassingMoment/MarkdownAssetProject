// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownAsset.h"
#include "MarkdownAssetHtmlExporter.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownHtmlExportStandaloneDocumentTest,
	"MarkdownAsset.Export.StandaloneHtml",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownHtmlExportStandaloneDocumentTest::RunTest(const FString& Parameters)
{
	const FName AssetName = MakeUniqueObjectName(GetTransientPackage(), UMarkdownAsset::StaticClass(), TEXT("ExportTestNote"));
	UMarkdownAsset* Asset = NewObject<UMarkdownAsset>(GetTransientPackage(), AssetName);
	Asset->RawMarkdownText = TEXT("# Title\n\nSome **bold** text & more.\n\n[[MissingNote_XyZ]]");

	const FString Html = UMarkdownAssetHtmlExporter::BuildStandaloneHtml(Asset, EMarkdownPreviewTheme::Light);

	TestTrue(TEXT("Document is a full HTML page"), Html.StartsWith(TEXT("<!DOCTYPE html>")));
	TestTrue(TEXT("Document declares UTF-8"), Html.Contains(TEXT("<meta charset=\"utf-8\">")));
	TestTrue(TEXT("Asset name is used as the title"),
		Html.Contains(FString::Printf(TEXT("<title>%s</title>"), *AssetName.ToString())));
	TestFalse(TEXT("Export omits the preview-only Content-Security-Policy"), Html.Contains(TEXT("Content-Security-Policy")));
	TestTrue(TEXT("Light theme background is applied"), Html.Contains(TEXT("background-color: #ffffff")));
	TestTrue(TEXT("Headings receive navigation anchors"), Html.Contains(TEXT("<h1 id=\"md-h-0\">")));
	TestTrue(TEXT("Markdown body is rendered"), Html.Contains(TEXT("<strong>bold</strong>")));
	TestTrue(TEXT("Unresolvable wikilinks are flagged"), Html.Contains(TEXT("md-broken-link")));

	TestTrue(TEXT("Null asset yields an empty document"),
		UMarkdownAssetHtmlExporter::BuildStandaloneHtml(nullptr, EMarkdownPreviewTheme::Dark).IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
