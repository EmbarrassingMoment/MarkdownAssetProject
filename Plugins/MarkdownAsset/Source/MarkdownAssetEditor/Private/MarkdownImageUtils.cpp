// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownImageUtils.h"
#include "MarkdownEditorSettings.h"
#include "MarkdownPreviewUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Internationalization/Regex.h"
#include "Misc/Base64.h"
#include "Misc/DateTime.h"
#include "Misc/ScopeLock.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogMarkdownImage, Log, All);

namespace
{
	/** Upper bound for an embedded PNG; larger images would bloat the preview data: URL. */
	constexpr int64 MaxEmbeddedPngBytes = 12 * 1024 * 1024;

	/** Upper bound for the total size of cached data: URIs before the cache is flushed. */
	constexpr int64 MaxTextureCacheBytes = 64 * 1024 * 1024;

	/** A PNG data: URI produced for a texture, tagged with the source data id it was built from. */
	struct FCachedTextureImage
	{
		FGuid SourceId;
		FString DataUri;
	};

	/** Cache of encoded textures keyed by package path; avoids re-encoding on every preview refresh. */
	TMap<FString, FCachedTextureImage> GTextureImageCache;
	int64 GTextureImageCacheBytes = 0;
	FCriticalSection GTextureImageCacheLock;

#if PLATFORM_WINDOWS
	/** Upper bound for clipboard image pixels (64 megapixels). */
	constexpr int64 MaxClipboardPixels = 64LL * 1024 * 1024;
#endif

	/** Returns true if the image src points at a UE content package. */
	bool IsTexturePackagePath(const FString& Src)
	{
		return Src.StartsWith(TEXT("/Game/"))
			|| Src.StartsWith(TEXT("/Engine/"))
			|| Src.StartsWith(TEXT("/Plugins/"));
	}

	/** Decodes HTML entities that md4c emits inside attribute values. */
	FString DecodeHtmlEntities(const FString& Input)
	{
		FString Output = Input;
		Output.ReplaceInline(TEXT("&amp;"), TEXT("&"));
		Output.ReplaceInline(TEXT("&#x2F;"), TEXT("/"));
		Output.ReplaceInline(TEXT("&#47;"), TEXT("/"));
		Output.ReplaceInline(TEXT("&lt;"), TEXT("<"));
		Output.ReplaceInline(TEXT("&gt;"), TEXT(">"));
		Output.ReplaceInline(TEXT("&quot;"), TEXT("\""));
		return Output;
	}

	/**
	 * Loads the UTexture2D at the given package path and compresses its source
	 * pixels to a data:image/png;base64 URI. Results are cached per package path
	 * and invalidated when the texture's source data id changes (reimport / edit).
	 */
	bool TryCreateTextureDataUri(const FString& PackagePath, FString& OutDataUri)
	{
		// Derive "/Game/Foo/Bar.Bar" from "/Game/Foo/Bar"; a path that already
		// carries an object name is used as-is.
		FString ObjectPath = PackagePath;
		int32 DotIndex;
		if (!ObjectPath.FindChar(TEXT('.'), DotIndex))
		{
			int32 SlashIndex;
			if (!ObjectPath.FindLastChar(TEXT('/'), SlashIndex) || SlashIndex + 1 >= ObjectPath.Len())
			{
				return false;
			}
			ObjectPath += TEXT(".") + ObjectPath.Mid(SlashIndex + 1);
		}

		// Avoid load attempts (and their log noise) for packages that do not exist.
		FString PackageOnly = ObjectPath;
		if (PackageOnly.FindChar(TEXT('.'), DotIndex))
		{
			PackageOnly.LeftInline(DotIndex);
		}
		if (!FPackageName::DoesPackageExist(PackageOnly))
		{
			MarkdownImageUtils::InvalidateTextureImageCache(PackagePath);
			return false;
		}

		UTexture2D* Texture = Cast<UTexture2D>(FSoftObjectPath(ObjectPath).TryLoad());
		if (!Texture || !Texture->Source.IsValid())
		{
			MarkdownImageUtils::InvalidateTextureImageCache(PackagePath);
			return false;
		}

		const FGuid SourceId = Texture->Source.GetId();
		{
			FScopeLock Lock(&GTextureImageCacheLock);
			if (const FCachedTextureImage* Cached = GTextureImageCache.Find(PackagePath))
			{
				if (Cached->SourceId == SourceId)
				{
					OutDataUri = Cached->DataUri;
					return true;
				}
			}
		}

		FImage SourceImage;
		if (!Texture->Source.GetMipImage(SourceImage, 0))
		{
			return false;
		}

		TArray64<uint8> PngData;
		if (!FImageUtils::CompressImage(PngData, TEXT("png"), SourceImage))
		{
			return false;
		}

		if (PngData.Num() > MaxEmbeddedPngBytes)
		{
			UE_LOG(LogMarkdownImage, Warning,
				TEXT("Texture '%s' is too large to embed in the preview (%lld bytes as PNG)"),
				*PackagePath, PngData.Num());
			return false;
		}

		OutDataUri = TEXT("data:image/png;base64,") + FBase64::Encode(PngData.GetData(), static_cast<uint32>(PngData.Num()));

		{
			FScopeLock Lock(&GTextureImageCacheLock);

			if (const FCachedTextureImage* Stale = GTextureImageCache.Find(PackagePath))
			{
				GTextureImageCacheBytes -= Stale->DataUri.Len() * sizeof(TCHAR);
			}

			// Keep the cache bounded: flush everything rather than tracking LRU order,
			// a full re-encode after 64 MB of previews is cheap enough.
			if (GTextureImageCacheBytes + OutDataUri.Len() * static_cast<int64>(sizeof(TCHAR)) > MaxTextureCacheBytes)
			{
				GTextureImageCache.Empty();
				GTextureImageCacheBytes = 0;
			}

			FCachedTextureImage& Entry = GTextureImageCache.FindOrAdd(PackagePath);
			Entry.SourceId = SourceId;
			Entry.DataUri = OutDataUri;
			GTextureImageCacheBytes += OutDataUri.Len() * sizeof(TCHAR);
		}

		return true;
	}

#if PLATFORM_WINDOWS
	/** Converts a packed DIB (CF_DIB) to top-down 8-bit BGRA pixels. */
	bool ConvertDibToBgra(const uint8* Data, SIZE_T DataSize, TArray<uint8>& OutBgra, int32& OutWidth, int32& OutHeight)
	{
		if (!Data || DataSize < sizeof(BITMAPINFOHEADER))
		{
			return false;
		}

		const BITMAPINFOHEADER* Header = reinterpret_cast<const BITMAPINFOHEADER*>(Data);
		if (Header->biSize < sizeof(BITMAPINFOHEADER))
		{
			return false;
		}

		const int32 Width = Header->biWidth;
		const int32 AbsHeight = FMath::Abs(Header->biHeight);
		const bool bBottomUp = Header->biHeight > 0;
		const uint16 BitsPerPixel = Header->biBitCount;

		if (Width <= 0 || AbsHeight <= 0
			|| static_cast<int64>(Width) * AbsHeight > MaxClipboardPixels
			|| (BitsPerPixel != 24 && BitsPerPixel != 32)
			|| (Header->biCompression != BI_RGB && Header->biCompression != BI_BITFIELDS))
		{
			return false;
		}

		// Pixel data follows the header, the color masks (only when a plain
		// BITMAPINFOHEADER uses BI_BITFIELDS; V4/V5 headers carry them inside
		// biSize), and any palette entries.
		SIZE_T PixelOffset = Header->biSize + static_cast<SIZE_T>(Header->biClrUsed) * 4;
		if (Header->biCompression == BI_BITFIELDS && Header->biSize == sizeof(BITMAPINFOHEADER))
		{
			PixelOffset += 12;
		}

		const SIZE_T Stride = ((static_cast<SIZE_T>(Width) * BitsPerPixel + 31) / 32) * 4;
		if (PixelOffset + Stride * AbsHeight > DataSize)
		{
			return false;
		}

		OutBgra.SetNumUninitialized(Width * AbsHeight * 4);

		bool bHasAlpha = (BitsPerPixel == 24);
		for (int32 Y = 0; Y < AbsHeight; ++Y)
		{
			const uint8* Row = Data + PixelOffset + Stride * (bBottomUp ? (AbsHeight - 1 - Y) : Y);
			uint8* Dest = OutBgra.GetData() + static_cast<SIZE_T>(Y) * Width * 4;

			if (BitsPerPixel == 32)
			{
				// Assume the standard BGRA channel order used by clipboard producers.
				FMemory::Memcpy(Dest, Row, static_cast<SIZE_T>(Width) * 4);
				for (int32 X = 0; X < Width; ++X)
				{
					bHasAlpha |= Dest[X * 4 + 3] != 0;
				}
			}
			else
			{
				for (int32 X = 0; X < Width; ++X)
				{
					Dest[X * 4 + 0] = Row[X * 3 + 0];
					Dest[X * 4 + 1] = Row[X * 3 + 1];
					Dest[X * 4 + 2] = Row[X * 3 + 2];
					Dest[X * 4 + 3] = 255;
				}
			}
		}

		// Many producers write 32bpp DIBs with a zeroed alpha channel; a fully
		// transparent screenshot is never the intent, so treat it as opaque.
		if (!bHasAlpha)
		{
			for (int64 Index = 3; Index < OutBgra.Num(); Index += 4)
			{
				OutBgra[Index] = 255;
			}
		}

		OutWidth = Width;
		OutHeight = AbsHeight;
		return true;
	}
#endif // PLATFORM_WINDOWS
}

FString MarkdownImageUtils::EmbedTextureImages(const FString& Html)
{
	// md4c always emits the src attribute first: <img src="..." alt="...">
	const FRegexPattern ImgPattern(TEXT("<img src=\"([^\"]*)\"([^>]*)>"));
	FRegexMatcher Matcher(ImgPattern, Html);

	FString Result;
	int32 LastPos = 0;

	while (Matcher.FindNext())
	{
		Result += Html.Mid(LastPos, Matcher.GetMatchBeginning() - LastPos);

		const FString Src = DecodeHtmlEntities(Matcher.GetCaptureGroup(1));
		const FString RestOfTag = Matcher.GetCaptureGroup(2);

		if (IsTexturePackagePath(Src))
		{
			FString DataUri;
			if (TryCreateTextureDataUri(Src, DataUri))
			{
				Result += FString::Printf(TEXT("<img src=\"%s\"%s>"), *DataUri, *RestOfTag);
			}
			else
			{
				Result += FString::Printf(TEXT("<span class=\"md-missing-image\">[Image not found: %s]</span>"), *MarkdownPreviewUtils::EscapeHtml(Src));
			}
		}
		else
		{
			Result += Html.Mid(Matcher.GetMatchBeginning(), Matcher.GetMatchEnding() - Matcher.GetMatchBeginning());
		}

		LastPos = Matcher.GetMatchEnding();
	}
	Result += Html.Mid(LastPos);

	return Result;
}

bool MarkdownImageUtils::ReadClipboardImage(TArray<uint8>& OutBgra, int32& OutWidth, int32& OutHeight)
{
#if PLATFORM_WINDOWS
	bool bSuccess = false;

	if (!OpenClipboard(nullptr))
	{
		return false;
	}

	if (HANDLE DibHandle = GetClipboardData(CF_DIB))
	{
		if (const void* DibData = GlobalLock(DibHandle))
		{
			bSuccess = ConvertDibToBgra(static_cast<const uint8*>(DibData), GlobalSize(DibHandle), OutBgra, OutWidth, OutHeight);
			GlobalUnlock(DibHandle);
		}
	}

	CloseClipboard();
	return bSuccess;
#else
	return false;
#endif
}

UTexture2D* MarkdownImageUtils::CreateTextureAssetFromBgra(const TArray<uint8>& Bgra, int32 Width, int32 Height, FString& OutPackagePath)
{
	if (Width <= 0 || Height <= 0 || Bgra.Num() != static_cast<int64>(Width) * Height * 4)
	{
		return nullptr;
	}

	const UMarkdownEditorSettings* Settings = GetDefault<UMarkdownEditorSettings>();
	const FString Folder = Settings ? Settings->GetPastedImagePackagePath() : TEXT("/Game/Markdown/Images");
	const FString BaseName = FString::Printf(TEXT("%s/T_PastedImage_%s"), *Folder,
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString PackageName;
	FString AssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(BaseName, TEXT(""), PackageName, AssetName);

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		return nullptr;
	}

	UTexture2D* Texture = NewObject<UTexture2D>(Package, FName(*AssetName), RF_Public | RF_Standalone);
	Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, Bgra.GetData());
	Texture->SRGB = true;
	Texture->CompressionSettings = TC_Default;
	// Pasted screenshots are usually non-power-of-two; skip mip generation.
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->UpdateResource();
	Texture->PostEditChange();
	Texture->MarkPackageDirty();

	FAssetRegistryModule::AssetCreated(Texture);

	OutPackagePath = PackageName;
	return Texture;
}

void MarkdownImageUtils::InvalidateTextureImageCache(const FString& PackagePath)
{
	FScopeLock Lock(&GTextureImageCacheLock);
	if (const FCachedTextureImage* Entry = GTextureImageCache.Find(PackagePath))
	{
		GTextureImageCacheBytes -= Entry->DataUri.Len() * sizeof(TCHAR);
		GTextureImageCache.Remove(PackagePath);
	}
}

void MarkdownImageUtils::ClearTextureImageCache()
{
	FScopeLock Lock(&GTextureImageCacheLock);
	GTextureImageCache.Empty();
	GTextureImageCacheBytes = 0;
}

int32 MarkdownImageUtils::GetTextureImageCacheCount()
{
	FScopeLock Lock(&GTextureImageCacheLock);
	return GTextureImageCache.Num();
}
