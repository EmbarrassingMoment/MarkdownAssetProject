// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownOutline.h"
#include "MarkdownAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	int32 CountSubstring(const FString& Haystack, const FString& Needle)
	{
		int32 Count = 0;
		int32 Pos = Haystack.Find(Needle, ESearchCase::CaseSensitive);
		while (Pos != INDEX_NONE)
		{
			++Count;
			Pos = Haystack.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Pos + Needle.Len());
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlineAtxHeadingsTest,
	"MarkdownAsset.Outline.AtxHeadings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlineAtxHeadingsTest::RunTest(const FString& Parameters)
{
	const TArray<FMarkdownHeading> Headings =
		MarkdownOutline::ExtractHeadings(TEXT("# A\ntext\n## B\n### C"));

	if (!TestEqual(TEXT("Three headings found"), Headings.Num(), 3))
	{
		return false;
	}

	TestEqual(TEXT("First heading level"), Headings[0].Level, 1);
	TestEqual(TEXT("First heading text"), Headings[0].Text, FString(TEXT("A")));
	TestEqual(TEXT("First heading line"), Headings[0].LineIndex, 0);
	TestEqual(TEXT("First heading order"), Headings[0].HeadingIndex, 0);

	TestEqual(TEXT("Second heading level"), Headings[1].Level, 2);
	TestEqual(TEXT("Second heading line"), Headings[1].LineIndex, 2);

	TestEqual(TEXT("Third heading level"), Headings[2].Level, 3);
	TestEqual(TEXT("Third heading line"), Headings[2].LineIndex, 3);

	const TArray<FMarkdownHeading> ClosedHeadings = MarkdownOutline::ExtractHeadings(TEXT("## Title ##"));
	if (TestEqual(TEXT("Closed-style heading is found"), ClosedHeadings.Num(), 1))
	{
		TestEqual(TEXT("Closing hash run is stripped"), ClosedHeadings[0].Text, FString(TEXT("Title")));
	}

	TestEqual(TEXT("Hash without a following space is not a heading"),
		MarkdownOutline::ExtractHeadings(TEXT("#NotHeading")).Num(), 0);

	TestEqual(TEXT("Empty input yields no headings"),
		MarkdownOutline::ExtractHeadings(TEXT("")).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlineSetextHeadingsTest,
	"MarkdownAsset.Outline.SetextHeadings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlineSetextHeadingsTest::RunTest(const FString& Parameters)
{
	const TArray<FMarkdownHeading> Headings =
		MarkdownOutline::ExtractHeadings(TEXT("Title\n=====\nSub\n---"));

	if (!TestEqual(TEXT("Both setext headings found"), Headings.Num(), 2))
	{
		return false;
	}

	TestEqual(TEXT("Equals underline is H1"), Headings[0].Level, 1);
	TestEqual(TEXT("Setext heading uses the content line index"), Headings[0].LineIndex, 0);
	TestEqual(TEXT("Setext heading text"), Headings[0].Text, FString(TEXT("Title")));

	TestEqual(TEXT("Dash underline is H2"), Headings[1].Level, 2);
	TestEqual(TEXT("Second setext heading line"), Headings[1].LineIndex, 2);

	TestEqual(TEXT("Thematic break after a blank line is not a heading"),
		MarkdownOutline::ExtractHeadings(TEXT("para\n\n---\n")).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlineCodeBlockTest,
	"MarkdownAsset.Outline.CodeBlocksSkipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlineCodeBlockTest::RunTest(const FString& Parameters)
{
	const TArray<FMarkdownHeading> FenceHeadings =
		MarkdownOutline::ExtractHeadings(TEXT("```\n# code comment\n```\n# real"));

	if (TestEqual(TEXT("Heading inside a fence is skipped"), FenceHeadings.Num(), 1))
	{
		TestEqual(TEXT("Real heading text"), FenceHeadings[0].Text, FString(TEXT("real")));
		TestEqual(TEXT("Real heading line"), FenceHeadings[0].LineIndex, 3);
	}

	TestEqual(TEXT("Heading-like line in indented code is skipped"),
		MarkdownOutline::ExtractHeadings(TEXT("text\n\n    # code")).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlineBlockquoteTest,
	"MarkdownAsset.Outline.BlockquoteHeadings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlineBlockquoteTest::RunTest(const FString& Parameters)
{
	const TArray<FMarkdownHeading> Headings =
		MarkdownOutline::ExtractHeadings(TEXT("> # Quoted"));

	if (TestEqual(TEXT("Blockquoted heading is counted"), Headings.Num(), 1))
	{
		TestEqual(TEXT("Blockquoted heading level"), Headings[0].Level, 1);
		TestEqual(TEXT("Blockquoted heading text"), Headings[0].Text, FString(TEXT("Quoted")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlineAnchorInjectionTest,
	"MarkdownAsset.Outline.AnchorInjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlineAnchorInjectionTest::RunTest(const FString& Parameters)
{
	const FString Injected = MarkdownOutline::InjectHeadingAnchors(TEXT("<h1>A</h1>\n<hr>\n<h2>B</h2>"));

	TestTrue(TEXT("First heading gets anchor 0"),
		Injected.Contains(TEXT("<h1 id=\"md-h-0\">A</h1>")));
	TestTrue(TEXT("Second heading gets anchor 1"),
		Injected.Contains(TEXT("<h2 id=\"md-h-1\">B</h2>")));
	TestTrue(TEXT("<hr> tag is untouched"),
		Injected.Contains(TEXT("<hr>")));

	TestEqual(TEXT("HTML without headings is returned unchanged"),
		MarkdownOutline::InjectHeadingAnchors(TEXT("<p>plain</p>")), FString(TEXT("<p>plain</p>")));

	TestEqual(TEXT("Heading tag with attributes is left alone"),
		MarkdownOutline::InjectHeadingAnchors(TEXT("<h1 class=\"x\">A</h1>")), FString(TEXT("<h1 class=\"x\">A</h1>")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownOutlinePreviewAlignmentTest,
	"MarkdownAsset.Outline.PreviewAnchorAlignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownOutlinePreviewAlignmentTest::RunTest(const FString& Parameters)
{
	// The outline's heading count must match the number of <hN> tags md4c emits,
	// otherwise outline clicks would scroll the preview to the wrong heading.
	const FString Markdown = TEXT(
		"# Top\n"
		"\n"
		"Intro paragraph.\n"
		"\n"
		"Setext Title\n"
		"------------\n"
		"\n"
		"```\n"
		"# not a heading\n"
		"```\n"
		"\n"
		"> # Quoted heading\n"
		"\n"
		"| A |\n"
		"| --- |\n"
		"| 1 |\n"
		"\n"
		"- item\n"
		"\n"
		"### Deep\n");

	UMarkdownAsset* Asset = NewObject<UMarkdownAsset>();
	Asset->RawMarkdownText = Markdown;

	const int32 OutlineCount = MarkdownOutline::ExtractHeadings(Markdown).Num();
	const FString Injected = MarkdownOutline::InjectHeadingAnchors(Asset->GetParsedHTML());
	const int32 AnchorCount = CountSubstring(Injected, TEXT("id=\"md-h-"));

	TestEqual(TEXT("Outline heading count matches injected anchor count"), OutlineCount, AnchorCount);
	TestEqual(TEXT("Expected number of headings in the sample document"), OutlineCount, 4);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
