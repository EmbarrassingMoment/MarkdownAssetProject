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

FString MarkdownPreviewUtils::EscapeHtml(const FString& Input)
{
	FString Output = Input;
	Output.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Output.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Output.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	Output.ReplaceInline(TEXT("\""), TEXT("&quot;"));
	return Output;
}

namespace
{
	/** The set of colors that differ between the preview themes. */
	struct FPreviewPalette
	{
		const TCHAR* Background;
		const TCHAR* Text;
		const TCHAR* Heading;
		const TCHAR* Border;
		const TCHAR* CodeBackground;
		const TCHAR* Link;
		const TCHAR* TableHeaderBackground;
		const TCHAR* TableStripe;
		const TCHAR* QuoteBackground;
		const TCHAR* Strikethrough;
		const TCHAR* MarkdownLink;
		const TCHAR* MarkdownLinkHover;
		const TCHAR* AssetLink;
		const TCHAR* AssetLinkHover;
		const TCHAR* ClassLink;
		const TCHAR* ClassLinkHover;
		const TCHAR* Broken;
		const TCHAR* BrokenHover;
	};

	const FPreviewPalette DarkPalette = {
		TEXT("#1e1e1e"), TEXT("#cccccc"), TEXT("#ffffff"), TEXT("#444444"), TEXT("#2d2d2d"), TEXT("#3794ff"),
		TEXT("#333333"), TEXT("#2a2a2a"), TEXT("#252525"), TEXT("#888888"),
		TEXT("#4ec9b0"), TEXT("#6fe0c8"), TEXT("#dcdcaa"), TEXT("#f1e9a6"), TEXT("#c586c0"), TEXT("#d7a7d2"),
		TEXT("#f44747"), TEXT("#ff6b6b")
	};

	const FPreviewPalette LightPalette = {
		TEXT("#ffffff"), TEXT("#24292f"), TEXT("#1f2328"), TEXT("#d0d7de"), TEXT("#f6f8fa"), TEXT("#0969da"),
		TEXT("#f6f8fa"), TEXT("#f6f8fa"), TEXT("#f6f8fa"), TEXT("#6e7781"),
		TEXT("#1a7f64"), TEXT("#116a53"), TEXT("#9a6700"), TEXT("#7d5300"), TEXT("#8250df"), TEXT("#6639ba"),
		TEXT("#cf222e"), TEXT("#a40e26")
	};

	const FPreviewPalette& GetPalette(EMarkdownPreviewTheme Theme)
	{
		return Theme == EMarkdownPreviewTheme::Light ? LightPalette : DarkPalette;
	}
}

FString MarkdownPreviewUtils::GenerateStyledHtml(const FString& ParsedHtml)
{
	return GenerateStyledHtml(ParsedHtml, FStyledHtmlOptions());
}

FString MarkdownPreviewUtils::GenerateStyledHtml(const FString& ParsedHtml, const FStyledHtmlOptions& Options)
{
	const FPreviewPalette& P = GetPalette(Options.Theme);

	FString Head;
	Head += TEXT("<meta charset=\"utf-8\">\n");
	if (Options.bIncludeContentSecurityPolicy)
	{
		Head += TEXT("<meta http-equiv=\"Content-Security-Policy\""
			" content=\"default-src 'none'; style-src 'unsafe-inline'; img-src data:;\">\n");
	}
	if (!Options.Title.IsEmpty())
	{
		Head += FString::Printf(TEXT("<title>%s</title>\n"), *EscapeHtml(Options.Title));
	}

	const FString Css = FString::Printf(TEXT(
		"body { font-family: 'Segoe UI', 'Meiryo', 'Yu Gothic', sans-serif; background-color: %s; color: %s; padding: 20px; }\n"
		"h1, h2, h3, h4, h5, h6 { color: %s; border-bottom: 1px solid %s; padding-bottom: 5px; }\n"
		"code { background-color: %s; padding: 2px 4px; border-radius: 4px; }\n"
		"pre { background-color: %s; padding: 10px; border-radius: 4px; overflow-x: auto; }\n"
		"a { color: %s; }\n"
		"table { border-collapse: collapse; width: 100%%; margin-bottom: 20px; }\n"
		"th, td { border: 1px solid %s; padding: 8px 12px; text-align: left; }\n"
		"th { background-color: %s; color: %s; font-weight: bold; }\n"
		"tr:nth-child(even) { background-color: %s; }\n"
		"blockquote { border-left: 4px solid %s; margin: 10px 0; padding: 5px 15px; background-color: %s; }\n"
		"hr { border: none; border-top: 1px solid %s; margin: 20px 0; }\n"
		"img { max-width: 100%%; height: auto; }\n"
		"del { color: %s; }\n"
		"a[href^=\"mdasset://\"] { color: %s; text-decoration: none; border-bottom: 1px dashed %s; cursor: pointer; }\n"
		"a[href^=\"mdasset://\"]:hover { color: %s; border-bottom-style: solid; }\n"
		"a[href^=\"ueasset://\"] { color: %s; text-decoration: none; border-bottom: 1px dashed %s; cursor: pointer; }\n"
		"a[href^=\"ueasset://\"]:hover { color: %s; border-bottom-style: solid; }\n"
		"a[href^=\"class://\"] { color: %s; text-decoration: none; border-bottom: 1px dashed %s; cursor: pointer; }\n"
		"a[href^=\"class://\"]:hover { color: %s; border-bottom-style: solid; }\n"
		"a.md-broken-link { color: %s; border-bottom-color: %s; }\n"
		"a.md-broken-link:hover { color: %s; }\n"
		"span.md-missing-image { color: %s; border: 1px dashed %s; padding: 2px 6px; border-radius: 4px; }\n"),
		P.Background, P.Text,
		P.Heading, P.Border,
		P.CodeBackground,
		P.CodeBackground,
		P.Link,
		P.Border,
		P.TableHeaderBackground, P.Heading,
		P.TableStripe,
		P.Link, P.QuoteBackground,
		P.Border,
		P.Strikethrough,
		P.MarkdownLink, P.MarkdownLink,
		P.MarkdownLinkHover,
		P.AssetLink, P.AssetLink,
		P.AssetLinkHover,
		P.ClassLink, P.ClassLink,
		P.ClassLinkHover,
		P.Broken, P.Broken,
		P.BrokenHover,
		P.Broken, P.Broken);

	return FString::Printf(TEXT(
		"<!DOCTYPE html>\n"
		"<html><head>\n"
		"%s"
		"<style>\n"
		"%s"
		"</style></head><body>\n%s\n</body></html>"
	), *Head, *Css, *ParsedHtml);
}
