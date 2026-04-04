// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAsset.h"

extern "C" {
#include "md4c.h"
#include "md4c-html.h"
}

#include "Internationalization/Regex.h"

DEFINE_LOG_CATEGORY_STATIC(LogMarkdownAsset, Log, All);

/** Percent-encodes a string for use in a URI, preserving unreserved characters (RFC 3986). */
static FString PercentEncode(const FString& Input)
{
	FTCHARToUTF8 Utf8(*Input);
	const char* Data = Utf8.Get();
	int32 Len = Utf8.Length();

	FString Encoded;
	Encoded.Reserve(Len * 3);

	for (int32 i = 0; i < Len; ++i)
	{
		uint8 Ch = static_cast<uint8>(Data[i]);
		if ((Ch >= 'A' && Ch <= 'Z') || (Ch >= 'a' && Ch <= 'z') || (Ch >= '0' && Ch <= '9')
			|| Ch == '-' || Ch == '_' || Ch == '.' || Ch == '~')
		{
			Encoded.AppendChar(static_cast<TCHAR>(Ch));
		}
		else
		{
			Encoded += FString::Printf(TEXT("%%%02X"), Ch);
		}
	}
	return Encoded;
}

/**
 * Converts md4c-html wikilink elements to anchor tags with the mdasset:// scheme.
 * Input:  <x-wikilink data-target="Name">Name</x-wikilink>
 * Output: <a href="mdasset://EncodedName">Name</a>
 */
static FString PostProcessWikilinks(const FString& Html)
{
	FString Result = Html;

	// Replace opening <x-wikilink data-target="..."> tags with <a href="mdasset://...">
	const FRegexPattern OpenPattern(TEXT("<x-wikilink data-target=\"([^\"]*)\">"));
	FRegexMatcher Matcher(OpenPattern, Result);

	FString Processed;
	int32 LastPos = 0;

	while (Matcher.FindNext())
	{
		Processed += Result.Mid(LastPos, Matcher.GetMatchBeginning() - LastPos);

		FString Target = Matcher.GetCaptureGroup(1);
		FString EncodedTarget = PercentEncode(Target);
		Processed += FString::Printf(TEXT("<a href=\"mdasset://%s\">"), *EncodedTarget);

		LastPos = Matcher.GetMatchEnding();
	}
	Processed += Result.Mid(LastPos);
	Result = Processed;

	// Replace closing </x-wikilink> tags with </a>
	Result = Result.Replace(TEXT("</x-wikilink>"), TEXT("</a>"));

	return Result;
}

/**
 * Helper function for md4c-html callback to append output to a string.
 */
static void MarkdownHtmlProcessOutputCallback(const MD_CHAR* OutputData, MD_SIZE OutputSize, void* UserData)
{
	FString* OutputString = static_cast<FString*>(UserData);

	// Convert from UTF-8 to FString handling non-null-terminated strings safely
	FUTF8ToTCHAR Utf8Converter(OutputData, OutputSize);
	OutputString->AppendChars(Utf8Converter.Get(), Utf8Converter.Length());
}

/** Converts the raw Markdown text to HTML using the md4c library with GitHub dialect. */
FString UMarkdownAsset::GetParsedHTML() const
{
	FString OutputHtml;

	if (RawMarkdownText.IsEmpty())
	{
		return OutputHtml;
	}

	// Convert FString to UTF-8
	FTCHARToUTF8 Utf8String(*RawMarkdownText);
	const MD_CHAR* MdInput = Utf8String.Get();
	MD_SIZE MdSize = Utf8String.Length();

	// Parse markdown to HTML (disable raw HTML blocks/spans to prevent XSS)
	const unsigned MdFlags = MD_DIALECT_GITHUB | MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS | MD_FLAG_WIKILINKS;
	int Result = md_html(MdInput, MdSize, MarkdownHtmlProcessOutputCallback, &OutputHtml, MdFlags, 0);
	if (Result != 0)
	{
		UE_LOG(LogMarkdownAsset, Error, TEXT("md_html() failed to parse Markdown (error code: %d)"), Result);
		return FString();
	}

	OutputHtml = PostProcessWikilinks(OutputHtml);

	return OutputHtml;
}

/** Returns the raw Markdown source text without any processing. */
FString UMarkdownAsset::GetRawMarkdownText() const
{
	return RawMarkdownText;
}

/**
 * Callback data structure for plain text extraction via md_parse().
 */
struct FPlainTextExtractData
{
	FString OutputText;
	bool bNeedsSeparator = false;
};

/** No-op callback invoked when the parser enters a block element. */
static int PlainTextEnterBlock(MD_BLOCKTYPE Type, void* Detail, void* UserData)
{
	return 0;
}

/** Marks that a newline separator is needed after certain block-level elements. */
static int PlainTextLeaveBlock(MD_BLOCKTYPE Type, void* Detail, void* UserData)
{
	FPlainTextExtractData* Data = static_cast<FPlainTextExtractData*>(UserData);

	// Insert newline after block-level elements to preserve paragraph separation
	if (Type == MD_BLOCK_P || Type == MD_BLOCK_H || Type == MD_BLOCK_CODE
		|| Type == MD_BLOCK_LI || Type == MD_BLOCK_HR || Type == MD_BLOCK_TR)
	{
		Data->bNeedsSeparator = true;
	}

	return 0;
}

/** No-op callback invoked when the parser enters an inline span. */
static int PlainTextEnterSpan(MD_SPANTYPE Type, void* Detail, void* UserData)
{
	return 0;
}

/** No-op callback invoked when the parser leaves an inline span. */
static int PlainTextLeaveSpan(MD_SPANTYPE Type, void* Detail, void* UserData)
{
	return 0;
}

/** Appends text content to the output, handling line breaks and paragraph separation. */
static int PlainTextCallback(MD_TEXTTYPE Type, const MD_CHAR* Text, MD_SIZE Size, void* UserData)
{
	FPlainTextExtractData* Data = static_cast<FPlainTextExtractData*>(UserData);

	switch (Type)
	{
	case MD_TEXT_BR:
	case MD_TEXT_SOFTBR:
		Data->OutputText.AppendChar(TEXT('\n'));
		Data->bNeedsSeparator = false;
		break;

	case MD_TEXT_NORMAL:
	case MD_TEXT_CODE:
	case MD_TEXT_ENTITY:
	case MD_TEXT_LATEXMATH:
		if (Data->bNeedsSeparator && Data->OutputText.Len() > 0)
		{
			Data->OutputText.AppendChar(TEXT('\n'));
			Data->bNeedsSeparator = false;
		}
		{
			FUTF8ToTCHAR Utf8Converter(Text, Size);
			Data->OutputText.AppendChars(Utf8Converter.Get(), Utf8Converter.Length());
		}
		break;

	default:
		break;
	}

	return 0;
}

/** Strips all Markdown syntax and returns plain text by parsing with md4c callbacks. */
FString UMarkdownAsset::GetPlainText() const
{
	if (RawMarkdownText.IsEmpty())
	{
		return FString();
	}

	// Convert FString to UTF-8
	FTCHARToUTF8 Utf8String(*RawMarkdownText);
	const MD_CHAR* MdInput = Utf8String.Get();
	MD_SIZE MdSize = Utf8String.Length();

	// Set up the parser to extract text only (disable raw HTML blocks/spans)
	MD_PARSER Parser = {};
	Parser.abi_version = 0;
	Parser.flags = MD_DIALECT_GITHUB | MD_FLAG_NOHTMLBLOCKS | MD_FLAG_NOHTMLSPANS | MD_FLAG_WIKILINKS;
	Parser.enter_block = PlainTextEnterBlock;
	Parser.leave_block = PlainTextLeaveBlock;
	Parser.enter_span = PlainTextEnterSpan;
	Parser.leave_span = PlainTextLeaveSpan;
	Parser.text = PlainTextCallback;

	FPlainTextExtractData ExtractData;
	int Result = md_parse(MdInput, MdSize, &Parser, &ExtractData);
	if (Result != 0)
	{
		UE_LOG(LogMarkdownAsset, Error, TEXT("md_parse() failed to parse Markdown (error code: %d)"), Result);
		return FString();
	}

	return ExtractData.OutputText;
}
