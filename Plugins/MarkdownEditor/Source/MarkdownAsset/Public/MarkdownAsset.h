// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MarkdownAsset.generated.h"

/**
 * Custom asset for storing Markdown text and rendering it to HTML.
 */
UCLASS(BlueprintType)
class MARKDOWNASSET_API UMarkdownAsset : public UObject
{
	GENERATED_BODY()

public:
	/** The raw Markdown text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markdown")
	FString RawMarkdownText;

#if WITH_EDITORONLY_DATA
	/** The path to the file this asset was imported from. */
	UPROPERTY(VisibleAnywhere, Category = "Import Settings")
	FString SourceFilePath;
#endif

	/** Parses the raw Markdown text into an HTML string. */
	UFUNCTION(BlueprintCallable, Category = "Markdown")
	FString GetParsedHTML() const;

	/** Returns the raw Markdown text as-is. */
	UFUNCTION(BlueprintCallable, Category = "Markdown")
	FString GetRawMarkdownText() const;

	/** Returns the plain text with all Markdown symbols removed. */
	UFUNCTION(BlueprintCallable, Category = "Markdown")
	FString GetPlainText() const;
};
