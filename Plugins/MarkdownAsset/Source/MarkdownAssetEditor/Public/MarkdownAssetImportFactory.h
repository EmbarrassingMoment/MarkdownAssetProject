// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "MarkdownAssetImportFactory.generated.h"

/**
 * Factory for importing UMarkdownAsset objects.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkdownAssetImportFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	/** Initializes the factory to import .md and .markdown files. */
	UMarkdownAssetImportFactory();

	/** Creates a UMarkdownAsset from the imported text buffer content. */
	virtual UObject* FactoryCreateText(
		UClass* InClass, UObject* InParent, FName InName,
		EObjectFlags Flags, UObject* Context,
		const TCHAR* Type, const TCHAR*& Buffer,
		const TCHAR* BufferEnd, FFeedbackContext* Warn) override;

	/** Checks whether the given object can be reimported and outputs its source file path. */
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;

	/** Updates the stored reimport source file path for the given object. */
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;

	/** Reimports the Markdown asset by reloading its content from the source file. */
	virtual EReimportResult::Type Reimport(UObject* Obj) override;

	/** Returns the import priority for this factory. */
	virtual int32 GetPriority() const override;
};
