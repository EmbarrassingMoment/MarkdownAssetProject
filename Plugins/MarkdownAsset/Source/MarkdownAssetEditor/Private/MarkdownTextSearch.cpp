// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownTextSearch.h"

TArray<FMarkdownTextMatch> MarkdownTextSearch::FindAll(const FString& Text, const FString& Query, bool bMatchCase)
{
	TArray<FMarkdownTextMatch> Matches;
	if (Query.IsEmpty() || Text.IsEmpty())
	{
		return Matches;
	}

	const ESearchCase::Type SearchCase = bMatchCase ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

	int32 SearchFrom = 0;
	while (SearchFrom <= Text.Len() - Query.Len())
	{
		const int32 Found = Text.Find(Query, SearchCase, ESearchDir::FromStart, SearchFrom);
		if (Found == INDEX_NONE)
		{
			break;
		}

		FMarkdownTextMatch& Match = Matches.AddDefaulted_GetRef();
		Match.Start = Found;
		Match.Length = Query.Len();

		SearchFrom = Found + Query.Len();
	}

	return Matches;
}

int32 MarkdownTextSearch::FindNextMatchIndex(const TArray<FMarkdownTextMatch>& Matches, int32 FromOffset, bool bForward)
{
	if (Matches.Num() == 0)
	{
		return INDEX_NONE;
	}

	if (bForward)
	{
		for (int32 Index = 0; Index < Matches.Num(); ++Index)
		{
			if (Matches[Index].Start >= FromOffset)
			{
				return Index;
			}
		}
		return 0; // wrap to the first match
	}

	for (int32 Index = Matches.Num() - 1; Index >= 0; --Index)
	{
		if (Matches[Index].Start < FromOffset)
		{
			return Index;
		}
	}
	return Matches.Num() - 1; // wrap to the last match
}

FString MarkdownTextSearch::ReplaceAll(const FString& Text, const FString& Query, const FString& Replacement, bool bMatchCase, int32& OutReplacedCount)
{
	OutReplacedCount = 0;

	const TArray<FMarkdownTextMatch> Matches = FindAll(Text, Query, bMatchCase);
	if (Matches.Num() == 0)
	{
		return Text;
	}

	FString Result;
	Result.Reserve(Text.Len() + Matches.Num() * (Replacement.Len() - Query.Len()));

	int32 LastPos = 0;
	for (const FMarkdownTextMatch& Match : Matches)
	{
		Result += Text.Mid(LastPos, Match.Start - LastPos);
		Result += Replacement;
		LastPos = Match.End();
	}
	Result += Text.Mid(LastPos);

	OutReplacedCount = Matches.Num();
	return Result;
}

namespace
{
	/** Advances Index past the line break starting at Index ("\r\n" counts as one). */
	int32 SkipLineBreak(const FString& Text, int32 Index)
	{
		if (Text[Index] == TEXT('\r') && Index + 1 < Text.Len() && Text[Index + 1] == TEXT('\n'))
		{
			return Index + 2;
		}
		return Index + 1;
	}

	bool IsLineBreak(TCHAR Char)
	{
		return Char == TEXT('\n') || Char == TEXT('\r');
	}
}

int32 MarkdownTextSearch::TextLocationToOffset(const FString& Text, const FTextLocation& Location)
{
	const int32 TargetLine = FMath::Max(0, Location.GetLineIndex());

	int32 LineStart = 0;
	int32 CurrentLine = 0;
	int32 Index = 0;
	while (CurrentLine < TargetLine && Index < Text.Len())
	{
		if (IsLineBreak(Text[Index]))
		{
			Index = SkipLineBreak(Text, Index);
			LineStart = Index;
			++CurrentLine;
		}
		else
		{
			++Index;
		}
	}

	// Clamp the column to the current line so a stale location never spills into the next line.
	int32 LineEnd = LineStart;
	while (LineEnd < Text.Len() && !IsLineBreak(Text[LineEnd]))
	{
		++LineEnd;
	}

	return FMath::Clamp(LineStart + FMath::Max(0, Location.GetOffset()), LineStart, LineEnd);
}

FTextLocation MarkdownTextSearch::OffsetToTextLocation(const FString& Text, int32 Offset)
{
	const int32 ClampedOffset = FMath::Clamp(Offset, 0, Text.Len());

	int32 LineIndex = 0;
	int32 LineStart = 0;
	int32 Index = 0;
	while (Index < ClampedOffset)
	{
		if (IsLineBreak(Text[Index]))
		{
			const int32 Next = SkipLineBreak(Text, Index);
			if (Next > ClampedOffset)
			{
				// Offset points into the middle of a "\r\n" pair; treat it as the end of this line.
				break;
			}
			Index = Next;
			LineStart = Index;
			++LineIndex;
		}
		else
		{
			++Index;
		}
	}

	return FTextLocation(LineIndex, ClampedOffset - LineStart);
}
