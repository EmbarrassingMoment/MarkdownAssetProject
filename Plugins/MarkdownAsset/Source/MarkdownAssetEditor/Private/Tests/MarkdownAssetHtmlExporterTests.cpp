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
	// Untrusted Markdown can carry javascript: links (md4c does not filter URL
	// schemes), so the exported page must keep blocking script execution.
	TestTrue(TEXT("Export keeps a Content-Security-Policy"), Html.Contains(TEXT("Content-Security-Policy")));
	TestTrue(TEXT("Export CSP denies scripts by default"), Html.Contains(TEXT("default-src 'none'")));
	TestFalse(TEXT("Export CSP does not allow scripts"), Html.Contains(TEXT("script-src")));
	TestTrue(TEXT("Export CSP allows external images"), Html.Contains(TEXT("img-src data: http: https:")));
	TestTrue(TEXT("Light theme background is applied"), Html.Contains(TEXT("background-color: #ffffff")));
	TestTrue(TEXT("Headings receive navigation anchors"), Html.Contains(TEXT("<h1 id=\"md-h-0\">")));
	TestTrue(TEXT("Markdown body is rendered"), Html.Contains(TEXT("<strong>bold</strong>")));
	TestTrue(TEXT("Unresolvable wikilinks are flagged"), Html.Contains(TEXT("md-broken-link")));

	TestTrue(TEXT("Null asset yields an empty document"),
		UMarkdownAssetHtmlExporter::BuildStandaloneHtml(nullptr, EMarkdownPreviewTheme::Dark).IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
