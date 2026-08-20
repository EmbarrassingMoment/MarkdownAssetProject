// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownPreviewUtils.h"

MarkdownPreviewUtils::EPreviewUrlAction MarkdownPreviewUtils::ClassifyPreviewUrl(const FString& Url, FString* OutDecodedPayload)
{
	// Allow data: URLs for preview loading, and about:blank used by CEF during init.
	if (Url.StartsWith(TEXT("data:")) || Url.StartsWith(TEXT("about:")))
	{
		return EPreviewUrlAction::AllowInPage;
	}

	static const FString MdAssetScheme = TEXT("mdasset://");
	if (Url.StartsWith(MdAssetScheme))
	{
		if (OutDecodedPayload)
		{
			*OutDecodedPayload = PercentDecode(Url.Mid(MdAssetScheme.Len()));
		}
		return EPreviewUrlAction::OpenMarkdownAsset;
	}

	static const FString UEAssetScheme = TEXT("ueasset://");
	if (Url.StartsWith(UEAssetScheme))
	{
		if (OutDecodedPayload)
		{
			*OutDecodedPayload = PercentDecode(Url.Mid(UEAssetScheme.Len()));
		}
		return EPreviewUrlAction::OpenUnrealAsset;
	}

	static const FString ClassScheme = TEXT("class://");
	if (Url.StartsWith(ClassScheme))
	{
		if (OutDecodedPayload)
		{
			*OutDecodedPayload = PercentDecode(Url.Mid(ClassScheme.Len()));
		}
		return EPreviewUrlAction::OpenClass;
	}

	if (Url.StartsWith(TEXT("http://")) || Url.StartsWith(TEXT("https://")))
	{
		return EPreviewUrlAction::PromptExternal;
	}

	// Default-deny: unknown schemes (javascript:, file:, vbscript:, etc.) must never
	// navigate, to prevent script execution or local-file access from untrusted Markdown.
	return EPreviewUrlAction::Block;
}

FString MarkdownPreviewUtils::PercentDecode(const FString& Input)
{
	TArray<uint8> Bytes;
	Bytes.Reserve(Input.Len());

	for (int32 i = 0; i < Input.Len(); ++i)
	{
		if (Input[i] == TEXT('%') && i + 2 < Input.Len())
		{
			FString HexStr = Input.Mid(i + 1, 2);
			uint8 Value = static_cast<uint8>(FCString::Strtoi(*HexStr, nullptr, 16));
			Bytes.Add(Value);
			i += 2;
		}
		else if (Input[i] == TEXT('+'))
		{
			Bytes.Add(static_cast<uint8>(' '));
		}
		else
		{
			// ASCII range character
			Bytes.Add(static_cast<uint8>(Input[i] & 0xFF));
		}
	}

	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
	return FString(Converter.Length(), Converter.Get());
}

FString MarkdownPreviewUtils::GenerateStyledHtml(const FString& ParsedHtml)
{
	return FString::Printf(TEXT(
		"<!DOCTYPE html>\n"
		"<html><head>\n"
		"<meta charset=\"utf-8\">\n"
		"<meta http-equiv=\"Content-Security-Policy\""
		" content=\"default-src 'none'; style-src 'unsafe-inline'; img-src data:;\">\n"
		"<style>\n"
		"body { font-family: 'Segoe UI', 'Meiryo', 'Yu Gothic', sans-serif; background-color: #1e1e1e; color: #cccccc; padding: 20px; }\n"
		"h1, h2, h3, h4, h5, h6 { color: #ffffff; border-bottom: 1px solid #444; padding-bottom: 5px; }\n"
		"code { background-color: #2d2d2d; padding: 2px 4px; border-radius: 4px; }\n"
		"pre { background-color: #2d2d2d; padding: 10px; border-radius: 4px; overflow-x: auto; }\n"
		"a { color: #3794ff; }\n"
		"table { border-collapse: collapse; width: 100%%; margin-bottom: 20px; }\n"
		"th, td { border: 1px solid #444; padding: 8px 12px; text-align: left; }\n"
		"th { background-color: #333; color: #fff; font-weight: bold; }\n"
		"tr:nth-child(even) { background-color: #2a2a2a; }\n"
		"blockquote { border-left: 4px solid #3794ff; margin: 10px 0; padding: 5px 15px; background-color: #252525; }\n"
		"hr { border: none; border-top: 1px solid #444; margin: 20px 0; }\n"
		"img { max-width: 100%%; height: auto; }\n"
		"del { color: #888; }\n"
"a[href^=\"mdasset://\"] { color: #4ec9b0; text-decoration: none; border-bottom: 1px dashed #4ec9b0; cursor: pointer; }\n"
"a[href^=\"mdasset://\"]:hover { color: #6fe0c8; border-bottom-style: solid; }\n"
"a[href^=\"ueasset://\"] { color: #dcdcaa; text-decoration: none; border-bottom: 1px dashed #dcdcaa; cursor: pointer; }\n"
"a[href^=\"ueasset://\"]:hover { color: #f1e9a6; border-bottom-style: solid; }\n"
"a[href^=\"class://\"] { color: #c586c0; text-decoration: none; border-bottom: 1px dashed #c586c0; cursor: pointer; }\n"
"a[href^=\"class://\"]:hover { color: #d7a7d2; border-bottom-style: solid; }\n"
"a.md-broken-link { color: #f44747; border-bottom-color: #f44747; }\n"
"a.md-broken-link:hover { color: #ff6b6b; }\n"
"span.md-missing-image { color: #f44747; border: 1px dashed #f44747; padding: 2px 6px; border-radius: 4px; }\n"
		"</style></head><body>\n%s\n</body></html>"
	), *ParsedHtml);
}
