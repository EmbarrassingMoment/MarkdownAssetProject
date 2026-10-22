// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/TextLayout.h"

/** A single occurrence of the search query inside the editor text. */
struct FMarkdownTextMatch
{
	/** Zero-based character offset of the match in the full text. */
	int32 Start = 0;

	/** Length of the match in characters. */
	int32 Length = 0;

	int32 End() const { return Start + Length; }
};

/**
 * Pure string helpers behind the Find & Replace bar. They work on the flat
 * text returned by the editor widget and convert between character offsets
 * and the (line, column) locations the widget uses for cursor and selection.
 */
namespace MarkdownTextSearch
{
	/** Returns every non-overlapping occurrence of Query in Text, in document order. */
	TArray<FMarkdownTextMatch> FindAll(const FString& Text, const FString& Query, bool bMatchCase);

	/**
	 * Picks the match to select next relative to a cursor offset. Searching forward
	 * returns the first match starting at or after FromOffset; backward returns the
	 * last match starting before FromOffset. Both wrap around. Returns INDEX_NONE
	 * when Matches is empty.
	 */
	int32 FindNextMatchIndex(const TArray<FMarkdownTextMatch>& Matches, int32 FromOffset, bool bForward);

	/** Replaces every occurrence of Query with Replacement and reports how many were replaced. */
	FString ReplaceAll(const FString& Text, const FString& Query, const FString& Replacement, bool bMatchCase, int32& OutReplacedCount);

	/**
	 * Converts a (line, column) location to a character offset. Lines are split on
	 * "\r\n", "\n" or "\r" exactly as the editor's text layout does. The result is
	 * clamped to the text.
	 */
	int32 TextLocationToOffset(const FString& Text, const FTextLocation& Location);

	/** Converts a character offset to the (line, column) location used by the editor. */
	FTextLocation OffsetToTextLocation(const FString& Text, int32 Offset);
}
