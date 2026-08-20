// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

/**
 * Pure helpers backing the HTML preview browser: the navigation scheme
 * allowlist and the styled preview document generation. Kept free of any
 * widget or editor state so they can be covered by automation tests.
 */
namespace MarkdownPreviewUtils
{
	/** What the preview browser should do with a navigation request. */
	enum class EPreviewUrlAction : uint8
	{
		/** data: / about: — let the browser load it in-page. */
		AllowInPage,
		/** mdasset:// — open the target Markdown asset in its editor. */
		OpenMarkdownAsset,
		/** ueasset:// — open the target Content Browser asset. */
		OpenUnrealAsset,
		/** class:// — open the target C++ or Blueprint class. */
		OpenClass,
		/** http(s):// — confirm with the user, then open in the system browser. */
		PromptExternal,
		/** Anything else (javascript:, file:, vbscript:, ...) — deny navigation. */
		Block,
	};

	/**
	 * Classifies a navigation URL against the preview's scheme allowlist.
	 * Unknown schemes are denied by default. For the custom asset/class schemes
	 * the percent-decoded link target is written to OutDecodedPayload when provided.
	 */
	EPreviewUrlAction ClassifyPreviewUrl(const FString& Url, FString* OutDecodedPayload = nullptr);

	/** Decodes a percent-encoded URI string back to a regular FString. */
	FString PercentDecode(const FString& Input);

	/**
	 * Wraps the parsed HTML content in a full HTML document with dark-themed CSS
	 * styling and the Content-Security-Policy meta tag.
	 */
	FString GenerateStyledHtml(const FString& ParsedHtml);
}
