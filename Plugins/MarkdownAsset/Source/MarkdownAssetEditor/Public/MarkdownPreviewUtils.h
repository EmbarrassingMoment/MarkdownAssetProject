// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "MarkdownEditorSettings.h"

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

	/** Which Content-Security-Policy the generated document carries. */
	enum class EContentSecurityPolicy : uint8
	{
		/** In-editor preview: no scripts and no external requests; images only from data: URIs. */
		Preview,
		/** Standalone HTML export: still no scripts, but http(s) images may load in the user's browser. */
		StandaloneExport,
	};

	/** Options controlling how the parsed HTML is wrapped into a full document. */
	struct FStyledHtmlOptions
	{
		/** Color theme of the generated CSS. */
		EMarkdownPreviewTheme Theme = EMarkdownPreviewTheme::Dark;

		/**
		 * Content-Security-Policy emitted as a meta tag. Both policies block script
		 * execution (including javascript: links); the standalone export additionally
		 * allows http(s) images so that regular external images render in a browser.
		 */
		EContentSecurityPolicy ContentSecurityPolicy = EContentSecurityPolicy::Preview;

		/** Optional document title; emitted as <title> when non-empty. */
		FString Title;
	};

	/**
	 * Classifies a navigation URL against the preview's scheme allowlist.
	 * Unknown schemes are denied by default. For the custom asset/class schemes
	 * the percent-decoded link target is written to OutDecodedPayload when provided.
	 */
	EPreviewUrlAction ClassifyPreviewUrl(const FString& Url, FString* OutDecodedPayload = nullptr);

	/** Decodes a percent-encoded URI string back to a regular FString. */
	FString PercentDecode(const FString& Input);

	/** Escapes text for safe inclusion in HTML content or attribute values. */
	FString EscapeHtml(const FString& Input);

	/**
	 * Wraps the parsed HTML content in a full HTML document with the dark theme
	 * CSS and the Content-Security-Policy meta tag (the in-editor preview defaults).
	 */
	FString GenerateStyledHtml(const FString& ParsedHtml);

	/** Wraps the parsed HTML content in a full HTML document using the given options. */
	FString GenerateStyledHtml(const FString& ParsedHtml, const FStyledHtmlOptions& Options);
}
