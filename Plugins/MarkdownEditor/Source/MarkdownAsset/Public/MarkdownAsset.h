// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#if WITH_EDITORONLY_DATA
#include "EditorFramework/AssetImportData.h"
#endif
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

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import")
	TObjectPtr<UAssetImportData> AssetImportData;
#endif

	// UObject interface
	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
};
