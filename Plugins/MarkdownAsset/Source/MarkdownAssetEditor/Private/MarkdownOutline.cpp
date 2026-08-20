// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownOutline.h"

namespace
{
	/** Returns true if the line consists solely of Char (at least MinRun of them). */
	bool IsRunOfChar(const FString& Line, TCHAR Char, int32 MinRun)
	{
		if (Line.Len() < MinRun)
		{
			return false;
		}
		for (const TCHAR C : Line)
		{
			if (C != Char)
			{
				return false;
			}
		}
		return true;
	}

	/** Returns true if the line is a thematic break (***, ---, ___, with optional inner spaces). */
	bool IsThematicBreak(const FString& Line)
	{
		if (Line.IsEmpty())
		{
			return false;
		}
		const TCHAR Marker = Line[0];
		if (Marker != TEXT('*') && Marker != TEXT('-') && Marker != TEXT('_'))
		{
			return false;
		}
		int32 MarkerCount = 0;
		for (const TCHAR C : Line)
		{
			if (C == Marker)
			{
				++MarkerCount;
			}
			else if (C != TEXT(' ') && C != TEXT('\t'))
			{
				return false;
			}
		}
		return MarkerCount >= 3;
	}

	/** Parses an ATX heading (1-6 '#' plus whitespace or end of line); returns false otherwise. */
	bool TryParseAtxHeading(const FString& Line, int32& OutLevel, FString& OutText)
	{
		if (Line.IsEmpty() || Line[0] != TEXT('#'))
		{
			return false;
		}

		int32 HashCount = 0;
		while (HashCount < Line.Len() && Line[HashCount] == TEXT('#'))
		{
			++HashCount;
		}
		if (HashCount > 6 || (HashCount < Line.Len() && !FChar::IsWhitespace(Line[HashCount])))
		{
			return false;
		}

		FString Text = Line.Mid(HashCount).TrimStartAndEnd();

		// Strip an optional closing hash run ("## Title ##" -> "Title").
		int32 End = Text.Len();
		while (End > 0 && Text[End - 1] == TEXT('#'))
		{
			--End;
		}
		if (End < Text.Len() && (End == 0 || FChar::IsWhitespace(Text[End - 1])))
		{
			Text = Text.Left(End).TrimEnd();
		}

		OutLevel = HashCount;
		OutText = MoveTemp(Text);
		return true;
	}

	/** Returns true if the line starts a list item or blockquote (cannot be setext content). */
	bool IsListOrQuoteLine(const FString& Line)
	{
		if (Line.IsEmpty())
		{
			return false;
		}
		if (Line[0] == TEXT('>') || Line[0] == TEXT('|'))
		{
			return true;
		}
		if ((Line[0] == TEXT('-') || Line[0] == TEXT('*') || Line[0] == TEXT('+'))
			&& Line.Len() > 1 && (Line[1] == TEXT(' ') || Line[1] == TEXT('\t')))
		{
			return true;
		}
		// Ordered list: 1-9 digits followed by '.' or ')' and whitespace.
		int32 DigitCount = 0;
		while (DigitCount < Line.Len() && FChar::IsDigit(Line[DigitCount]))
		{
			++DigitCount;
		}
		if (DigitCount > 0 && DigitCount <= 9 && DigitCount + 1 < Line.Len()
			&& (Line[DigitCount] == TEXT('.') || Line[DigitCount] == TEXT(')'))
			&& (Line[DigitCount + 1] == TEXT(' ') || Line[DigitCount + 1] == TEXT('\t')))
		{
			return true;
		}
		return false;
	}
}

TArray<FMarkdownHeading> MarkdownOutline::ExtractHeadings(const FString& Markdown)
{
	TArray<FMarkdownHeading> Headings;
	if (Markdown.IsEmpty())
	{
		return Headings;
	}

	TArray<FString> Lines;
	Markdown.ParseIntoArrayLines(Lines, /*InCullEmpty*/ false);

	bool bInFence = false;
	TCHAR FenceChar = 0;
	int32 FenceLen = 0;

	// Whether the previous line is paragraph text that a setext underline may promote.
	bool bPrevIsParagraph = false;
	FString PrevLineText;

	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		const FString& RawLine = Lines[LineIndex];

		int32 Indent = 0;
		while (Indent < RawLine.Len() && RawLine[Indent] == TEXT(' '))
		{
			++Indent;
		}
		// Block-level markup (fences, ATX/setext headings) allows at most 3 spaces
		// of indentation; deeper indentation is an indented code block, or a lazy
		// paragraph continuation when it directly follows paragraph text.
		const bool bSmallIndent = Indent <= 3 && !(RawLine.Len() > 0 && RawLine[0] == TEXT('\t'));
		const FString Line = RawLine.Mid(Indent).TrimEnd();

		if (!bInFence && !bSmallIndent)
		{
			// Indented code (blank-separated) or paragraph continuation; never a heading.
			bPrevIsParagraph = bPrevIsParagraph && !Line.IsEmpty();
			if (bPrevIsParagraph)
			{
				PrevLineText = Line;
			}
			continue;
		}

		// Fenced code blocks (``` or ~~~); headings inside are ignored.
		if (bSmallIndent && Line.Len() >= 3 && (Line[0] == TEXT('`') || Line[0] == TEXT('~')))
		{
			const TCHAR C = Line[0];
			int32 Run = 0;
			while (Run < Line.Len() && Line[Run] == C)
			{
				++Run;
			}
			if (Run >= 3)
			{
				if (!bInFence)
				{
					bInFence = true;
					FenceChar = C;
					FenceLen = Run;
					bPrevIsParagraph = false;
					continue;
				}
				if (C == FenceChar && Run >= FenceLen && Run == Line.Len())
				{
					bInFence = false;
					bPrevIsParagraph = false;
					continue;
				}
			}
		}
		if (bInFence)
		{
			continue;
		}

		if (Line.IsEmpty())
		{
			bPrevIsParagraph = false;
			continue;
		}

		// ATX heading, either at top level or nested in a blockquote ("> # Title").
		// Blockquoted headings still emit <hN> tags in the preview, so they must be
		// counted to keep anchor indices aligned.
		{
			FString Content = Line;
			while (!Content.IsEmpty() && Content[0] == TEXT('>'))
			{
				Content.MidInline(1);
				Content.TrimStartInline();
			}

			int32 Level = 0;
			FString Text;
			if (TryParseAtxHeading(Content, Level, Text))
			{
				FMarkdownHeading Heading;
				Heading.Level = Level;
				Heading.Text = MoveTemp(Text);
				Heading.LineIndex = LineIndex;
				Heading.HeadingIndex = Headings.Num();
				Headings.Add(MoveTemp(Heading));

				bPrevIsParagraph = false;
				continue;
			}
		}

		// Setext underline: a run of '=' (H1) or '-' (H2) below paragraph text.
		if (bPrevIsParagraph
			&& (IsRunOfChar(Line, TEXT('='), 1) || IsRunOfChar(Line, TEXT('-'), 1)))
		{
			FMarkdownHeading Heading;
			Heading.Level = (Line[0] == TEXT('=')) ? 1 : 2;
			Heading.Text = PrevLineText;
			Heading.LineIndex = LineIndex - 1;
			Heading.HeadingIndex = Headings.Num();
			Headings.Add(MoveTemp(Heading));

			bPrevIsParagraph = false;
			continue;
		}

		// Ordinary line: usable as setext content unless it is structural markup.
		if (IsThematicBreak(Line) || IsListOrQuoteLine(Line))
		{
			bPrevIsParagraph = false;
		}
		else
		{
			bPrevIsParagraph = true;
			PrevLineText = Line;
		}
	}

	return Headings;
}

FString MarkdownOutline::InjectHeadingAnchors(const FString& Html)
{
	FString Result;
	Result.Reserve(Html.Len() + 64);

	int32 Pos = 0;
	int32 HeadingIndex = 0;

	while (true)
	{
		const int32 Found = Html.Find(TEXT("<h"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Pos);
		if (Found == INDEX_NONE)
		{
			break;
		}

		// Only rewrite a bare "<hN>" opening tag (md4c emits headings without attributes).
		if (Found + 3 < Html.Len()
			&& Html[Found + 2] >= TEXT('1') && Html[Found + 2] <= TEXT('6')
			&& Html[Found + 3] == TEXT('>'))
		{
			Result += Html.Mid(Pos, Found - Pos);
			Result += FString::Printf(TEXT("<h%c id=\"md-h-%d\">"), Html[Found + 2], HeadingIndex++);
			Pos = Found + 4;
		}
		else
		{
			Result += Html.Mid(Pos, Found + 2 - Pos);
			Pos = Found + 2;
		}
	}

	Result += Html.Mid(Pos);
	return Result;
}
