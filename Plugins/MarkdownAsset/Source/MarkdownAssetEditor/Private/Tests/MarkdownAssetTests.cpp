// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString ParseMarkdown(const FString& Markdown)
	{
		UMarkdownAsset* Asset = NewObject<UMarkdownAsset>();
		Asset->RawMarkdownText = Markdown;
		return Asset->GetParsedHTML();
	}

	FString ExtractPlainText(const FString& Markdown)
	{
		UMarkdownAsset* Asset = NewObject<UMarkdownAsset>();
		Asset->RawMarkdownText = Markdown;
		return Asset->GetPlainText();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetParsedHtmlBasicsTest,
	"MarkdownAsset.Asset.ParsedHtml.Basics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetParsedHtmlBasicsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Empty markdown yields empty HTML"),
		ParseMarkdown(TEXT("")).IsEmpty());

	TestTrue(TEXT("ATX heading renders as <h1>"),
		ParseMarkdown(TEXT("# Hello")).Contains(TEXT("<h1>Hello</h1>")));

	TestTrue(TEXT("Bold renders as <strong>"),
		ParseMarkdown(TEXT("**bold**")).Contains(TEXT("<strong>bold</strong>")));

	TestTrue(TEXT("Multibyte text survives the UTF-8 round trip"),
		ParseMarkdown(TEXT("# こんにちは")).Contains(TEXT("こんにちは")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetParsedHtmlGfmTest,
	"MarkdownAsset.Asset.ParsedHtml.GitHubFlavoredMarkdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetParsedHtmlGfmTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Strikethrough renders as <del>"),
		ParseMarkdown(TEXT("~~gone~~")).Contains(TEXT("<del>gone</del>")));

	TestTrue(TEXT("Table renders as <table>"),
		ParseMarkdown(TEXT("| A |\n| --- |\n| 1 |")).Contains(TEXT("<table>")));

	TestTrue(TEXT("Task list renders a checkbox input"),
		ParseMarkdown(TEXT("- [x] done")).Contains(TEXT("checkbox")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetParsedHtmlSecurityTest,
	"MarkdownAsset.Asset.ParsedHtml.RawHtmlEscaping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetParsedHtmlSecurityTest::RunTest(const FString& Parameters)
{
	// Raw HTML blocks and spans are disabled (MD_FLAG_NOHTMLBLOCKS / NOHTMLSPANS),
	// so script tags in the source must come out escaped, never as live markup.
	const FString BlockHtml = ParseMarkdown(TEXT("<script>alert('x')</script>"));
	TestFalse(TEXT("Script block is not emitted as live markup"),
		BlockHtml.Contains(TEXT("<script>")));
	TestTrue(TEXT("Script block is escaped as text"),
		BlockHtml.Contains(TEXT("&lt;script")));

	const FString InlineHtml = ParseMarkdown(TEXT("hello <img src=x onerror=alert(1)> world"));
	TestFalse(TEXT("Inline HTML is not emitted as live markup"),
		InlineHtml.Contains(TEXT("<img")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetWikilinkTest,
	"MarkdownAsset.Asset.ParsedHtml.Wikilinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetWikilinkTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Wikilink rewrites to the mdasset:// scheme"),
		ParseMarkdown(TEXT("[[MyNote]]")).Contains(TEXT("<a href=\"mdasset://MyNote\">MyNote</a>")));

	TestTrue(TEXT("Wikilink target with spaces is percent-encoded"),
		ParseMarkdown(TEXT("[[My Note]]")).Contains(TEXT("mdasset://My%20Note")));

	TestTrue(TEXT("Multibyte wikilink target is percent-encoded as UTF-8"),
		ParseMarkdown(TEXT("[[ノート]]")).Contains(TEXT("mdasset://%E3%83%8E%E3%83%BC%E3%83%88")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetAssetAndClassLinkTest,
	"MarkdownAsset.Asset.ParsedHtml.AssetAndClassLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetAssetAndClassLinkTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("/Game/ package path rewrites to ueasset://"),
		ParseMarkdown(TEXT("[Hero](/Game/Characters/Hero)")).Contains(TEXT("href=\"ueasset:///Game/Characters/Hero\"")));

	TestTrue(TEXT("/Engine/ package path rewrites to ueasset://"),
		ParseMarkdown(TEXT("[Cube](/Engine/BasicShapes/Cube)")).Contains(TEXT("href=\"ueasset:///Engine/BasicShapes/Cube\"")));

	TestTrue(TEXT("class:// link is preserved and normalized"),
		ParseMarkdown(TEXT("[Actor](class://AActor)")).Contains(TEXT("href=\"class://AActor\"")));

	TestTrue(TEXT("External http(s) link is left untouched"),
		ParseMarkdown(TEXT("[site](https://example.com)")).Contains(TEXT("href=\"https://example.com\"")));

	TestFalse(TEXT("Absolute path outside package roots is not rewritten"),
		ParseMarkdown(TEXT("[other](/Other/Path)")).Contains(TEXT("ueasset")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownAssetPlainTextTest,
	"MarkdownAsset.Asset.PlainText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownAssetPlainTextTest::RunTest(const FString& Parameters)
{
	const FString Plain = ExtractPlainText(TEXT("# Hello **World**"));
	TestTrue(TEXT("Plain text keeps the words"), Plain.Contains(TEXT("Hello World")));
	TestFalse(TEXT("Plain text drops heading markers"), Plain.Contains(TEXT("#")));
	TestFalse(TEXT("Plain text drops emphasis markers"), Plain.Contains(TEXT("*")));

	TestTrue(TEXT("Empty markdown yields empty plain text"),
		ExtractPlainText(TEXT("")).IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
