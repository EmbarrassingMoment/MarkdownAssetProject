// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAsset.h"

extern "C" {
#include "md4c-html.h"
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

	// Parse markdown to HTML
	md_html(MdInput, MdSize, MarkdownHtmlProcessOutputCallback, &OutputHtml, MD_DIALECT_GITHUB, 0);

	return OutputHtml;
}

void UMarkdownAsset::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (!AssetImportData)
		{
			AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
		}
	}
#endif
}

void UMarkdownAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);

#if WITH_EDITORONLY_DATA
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(
			SourceFileTagName(),
			AssetImportData->GetSourceData().ToJson(),
			FAssetRegistryTag::TT_Hidden));
	}
#endif
}
