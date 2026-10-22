// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownTextSearch.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownTextSearchFindAllTest,
	"MarkdownAsset.Search.FindAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownTextSearchFindAllTest::RunTest(const FString& Parameters)
{
	const FString Text = TEXT("Foo foo FOO\nfoofoo");

	const TArray<FMarkdownTextMatch> CaseInsensitive = MarkdownTextSearch::FindAll(Text, TEXT("foo"), false);
	TestEqual(TEXT("Case-insensitive search finds every occurrence"), CaseInsensitive.Num(), 5);
	if (CaseInsensitive.Num() == 5)
	{
		TestEqual(TEXT("First match starts at 0"), CaseInsensitive[0].Start, 0);
		TestEqual(TEXT("Match length equals the query length"), CaseInsensitive[0].Length, 3);
		TestEqual(TEXT("Adjacent occurrences are both found"), CaseInsensitive[4].Start, 15);
	}

	const TArray<FMarkdownTextMatch> CaseSensitive = MarkdownTextSearch::FindAll(Text, TEXT("foo"), true);
	TestEqual(TEXT("Case-sensitive search skips differently cased text"), CaseSensitive.Num(), 3);

	TestEqual(TEXT("Empty query yields no matches"),
		MarkdownTextSearch::FindAll(Text, TEXT(""), false).Num(), 0);
	TestEqual(TEXT("Query longer than the text yields no matches"),
		MarkdownTextSearch::FindAll(TEXT("ab"), TEXT("abc"), false).Num(), 0);
	TestEqual(TEXT("Matches do not overlap"),
		MarkdownTextSearch::FindAll(TEXT("aaaa"), TEXT("aa"), false).Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownTextSearchNavigationTest,
	"MarkdownAsset.Search.NextPreviousWrapAround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownTextSearchNavigationTest::RunTest(const FString& Parameters)
{
	const TArray<FMarkdownTextMatch> Matches = MarkdownTextSearch::FindAll(TEXT("a-a-a"), TEXT("a"), false);
	TestEqual(TEXT("Three matches at 0, 2, 4"), Matches.Num(), 3);

	TestEqual(TEXT("Forward from the start picks the first match"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 0, true), 0);
	TestEqual(TEXT("Forward from just after the first match picks the second"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 1, true), 1);
	TestEqual(TEXT("Forward past the last match wraps to the first"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 5, true), 0);

	TestEqual(TEXT("Backward from the end picks the last match"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 5, false), 2);
	TestEqual(TEXT("Backward from the second match picks the first"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 2, false), 0);
	TestEqual(TEXT("Backward from the first match wraps to the last"),
		MarkdownTextSearch::FindNextMatchIndex(Matches, 0, false), 2);

	TestEqual(TEXT("No matches yields INDEX_NONE"),
		MarkdownTextSearch::FindNextMatchIndex(TArray<FMarkdownTextMatch>(), 0, true), static_cast<int32>(INDEX_NONE));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownTextSearchReplaceAllTest,
	"MarkdownAsset.Search.ReplaceAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownTextSearchReplaceAllTest::RunTest(const FString& Parameters)
{
	int32 Count = 0;

	TestEqual(TEXT("Every occurrence is replaced"),
		MarkdownTextSearch::ReplaceAll(TEXT("cat Cat cat"), TEXT("cat"), TEXT("dog"), false, Count), FString(TEXT("dog dog dog")));
	TestEqual(TEXT("Replacement count is reported"), Count, 3);

	TestEqual(TEXT("Case-sensitive replace leaves other casings alone"),
		MarkdownTextSearch::ReplaceAll(TEXT("cat Cat cat"), TEXT("cat"), TEXT("dog"), true, Count), FString(TEXT("dog Cat dog")));
	TestEqual(TEXT("Case-sensitive count"), Count, 2);

	TestEqual(TEXT("Replacement may be longer than the query"),
		MarkdownTextSearch::ReplaceAll(TEXT("a.b"), TEXT("."), TEXT("::"), false, Count), FString(TEXT("a::b")));
	TestEqual(TEXT("Replacement may be empty"),
		MarkdownTextSearch::ReplaceAll(TEXT("a.b"), TEXT("."), TEXT(""), false, Count), FString(TEXT("ab")));

	const FString Unchanged = TEXT("nothing here");
	TestEqual(TEXT("No match returns the input unchanged"),
		MarkdownTextSearch::ReplaceAll(Unchanged, TEXT("zzz"), TEXT("y"), false, Count), Unchanged);
	TestEqual(TEXT("No match reports zero"), Count, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownTextSearchLocationConversionTest,
	"MarkdownAsset.Search.OffsetLocationConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownTextSearchLocationConversionTest::RunTest(const FString& Parameters)
{
	// Unix line endings.
	{
		const FString Text = TEXT("ab\ncd\n\nef");
		const FTextLocation Loc = MarkdownTextSearch::OffsetToTextLocation(Text, 4);
		TestEqual(TEXT("Offset 4 is line 1"), Loc.GetLineIndex(), 1);
		TestEqual(TEXT("Offset 4 is column 1"), Loc.GetOffset(), 1);
		TestEqual(TEXT("Round trip (1,1) -> 4"), MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(1, 1)), 4);

		const FTextLocation EmptyLine = MarkdownTextSearch::OffsetToTextLocation(Text, 6);
		TestEqual(TEXT("Offset 6 is the empty line 2"), EmptyLine.GetLineIndex(), 2);
		TestEqual(TEXT("Offset 6 is column 0"), EmptyLine.GetOffset(), 0);

		TestEqual(TEXT("Line 3 column 1 -> 8"), MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(3, 1)), 8);
		TestEqual(TEXT("Column past the line end is clamped to the line"),
			MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(0, 99)), 2);
		TestEqual(TEXT("Line past the end is clamped to the text length"),
			MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(99, 0)), Text.Len());
	}

	// Windows line endings: "\r\n" is a single line break.
	{
		const FString Text = TEXT("ab\r\ncd");
		const FTextLocation Loc = MarkdownTextSearch::OffsetToTextLocation(Text, 5);
		TestEqual(TEXT("CRLF: offset 5 is line 1"), Loc.GetLineIndex(), 1);
		TestEqual(TEXT("CRLF: offset 5 is column 1"), Loc.GetOffset(), 1);
		TestEqual(TEXT("CRLF: (1,0) -> 4"), MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(1, 0)), 4);
		TestEqual(TEXT("CRLF: (0,2) stays on line 0 before the break"),
			MarkdownTextSearch::TextLocationToOffset(Text, FTextLocation(0, 2)), 2);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
