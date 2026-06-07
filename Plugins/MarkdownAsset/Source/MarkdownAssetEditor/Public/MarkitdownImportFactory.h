// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"
#include "MarkitdownImportFactory.generated.h"

/**
 * Factory that imports binary / non-Markdown source files (PDF, DOCX, PPTX,
 * HTML) by routing them through markitdown and storing the resulting Markdown
 * inside a new UMarkdownAsset. Reimport re-runs markitdown against the
 * original source file.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkitdownImportFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UMarkitdownImportFactory();

	virtual UObject* FactoryCreateFile(
		UClass* InClass, UObject* InParent, FName InName,
		EObjectFlags Flags, const FString& Filename, const TCHAR* Parms,
		FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
};
