// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/**
 * Image support for the Markdown preview: embedding project textures referenced
 * with ![alt](/Game/Path/To/Texture) as data: URIs, and creating texture assets
 * from clipboard images. The preview's Content-Security-Policy only allows
 * img-src data:, so textures are inlined rather than served from disk.
 */
namespace MarkdownImageUtils
{
	/**
	 * Rewrites <img> tags whose src is a UE package path (/Game/, /Engine/,
	 * /Plugins/) into data:image/png;base64 URIs by loading the referenced
	 * UTexture2D and compressing its source pixels to PNG. Unresolvable package
	 * paths are replaced by a styled placeholder span; all other src values
	 * (http, data:, relative paths) are left untouched.
	 */
	FString EmbedTextureImages(const FString& Html);

	/**
	 * Drops the cached data: URI for a single texture package path. Entries are
	 * also refreshed automatically when the texture's source data id changes.
	 */
	void InvalidateTextureImageCache(const FString& PackagePath);

	/** Drops every cached data: URI (e.g. on editor shutdown or from tests). */
	void ClearTextureImageCache();

	/** Number of textures currently held in the encode cache. */
	int32 GetTextureImageCacheCount();

	/**
	 * Reads an image from the OS clipboard as 8-bit BGRA pixels.
	 * Windows-only (CF_DIB); returns false on other platforms or when the
	 * clipboard holds no usable image.
	 */
	bool ReadClipboardImage(TArray<uint8>& OutBgra, int32& OutWidth, int32& OutHeight);

	/**
	 * Creates a new UTexture2D asset from 8-bit BGRA pixels under the folder
	 * configured in Editor Preferences > Plugins > Markdown Editor (default
	 * /Game/Markdown/Images/; a unique name is generated). Returns the created
	 * texture and writes its package path (e.g. /Game/Markdown/Images/T_PastedImage_...)
	 * to OutPackagePath, or returns nullptr on failure.
	 */
	UTexture2D* CreateTextureAssetFromBgra(const TArray<uint8>& Bgra, int32 Width, int32 Height, FString& OutPackagePath);
}
