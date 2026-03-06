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

	/** Parses the raw Markdown text into an HTML string. */
	UFUNCTION(BlueprintCallable, Category = "Markdown")
	FString GetParsedHTML() const;
};
