// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MarkitdownBlueprintLibrary.generated.h"

class UMarkdownAsset;

/**
 * Blueprint-callable helpers for converting external files into UMarkdownAsset
 * via markitdown. Intended for use from Editor Utility Widgets and editor
 * scripts; not safe to call at runtime.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkitdownBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Opens an OS file picker filtered to markitdown-supported formats. Returns the selected absolute paths. */
	UFUNCTION(BlueprintCallable, Category = "Markitdown", meta = (DisplayName = "Prompt For Source Files"))
	static TArray<FString> PromptForSourceFiles();

	/**
	 * Converts a single file synchronously and creates a UMarkdownAsset at
	 * OutPackagePath/AssetName. If AssetName is empty the source basename is used.
	 * Returns the created asset or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Markitdown")
	static UMarkdownAsset* ConvertFileToMarkdownAsset(const FString& InputFilePath, const FString& OutPackagePath, FString AssetName);

	/**
	 * Converts a batch of files synchronously with a cancellable progress dialog.
	 * Returns the successfully created assets.
	 */
	UFUNCTION(BlueprintCallable, Category = "Markitdown")
	static TArray<UMarkdownAsset*> ConvertFilesToMarkdownAssets(const TArray<FString>& InputFilePaths, const FString& OutPackagePath);

	/** End-to-end wizard: prompt for files, batch-convert into OutPackagePath, and show a notification. */
	UFUNCTION(BlueprintCallable, Category = "Markitdown")
	static void RunBatchConvertWizard(const FString& OutPackagePath);
};
