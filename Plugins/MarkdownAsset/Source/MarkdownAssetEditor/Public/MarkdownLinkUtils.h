// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

/**
 * Helpers for the asset / class / wikilink schemes: resolving whether a link
 * target exists (to flag broken links in the preview) and producing the
 * Markdown link syntax for an asset dropped into the editor.
 */
namespace MarkdownLinkUtils
{
	/**
	 * Builds reflection-name candidates to try when resolving a user-supplied class name.
	 * UHT strips the leading A/U/I prefix from native class names, so "class://AActor"
	 * must be matched against the UClass named "Actor", and a bare "Actor" may need
	 * the prefixed forms as well.
	 */
	TArray<FString> BuildClassNameCandidates(const FString& ClassName);

	/** Returns true if an Unreal asset exists at the given object or package path. */
	bool DoesAssetExistAtPath(const FString& ObjectPath);

	/** Returns true if a native UClass or Blueprint class matching ClassName can be resolved. */
	bool DoesClassExist(const FString& ClassName);

	/**
	 * Walks every <a href="..."> tag and adds the "md-broken-link" CSS class when
	 * the target of an mdasset://, ueasset://, or class:// scheme cannot be resolved.
	 * External URLs and other schemes are left untouched.
	 */
	FString MarkBrokenLinks(const FString& Html);

	/**
	 * Returns the Markdown snippet that links to an asset, chosen by its class:
	 * Markdown assets become wikilinks ([[Name]]), textures become image links
	 * (![Name](/Game/Path/Name)), and everything else becomes an asset link
	 * ([Name](/Game/Path/Name)). AssetClass may be null, in which case the
	 * generic asset link is produced.
	 */
	FString BuildMarkdownLinkForAsset(const FString& PackageName, const FString& AssetName, const UClass* AssetClass);
}
