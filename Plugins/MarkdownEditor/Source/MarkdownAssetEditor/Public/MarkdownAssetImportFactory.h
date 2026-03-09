// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MarkdownAssetImportFactory.generated.h"

/**
 * Factory for importing UMarkdownAsset objects.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkdownAssetImportFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMarkdownAssetImportFactory();

	// UFactory interface
	virtual UObject* FactoryCreateText(
		UClass* InClass, UObject* InParent, FName InName,
		EObjectFlags Flags, UObject* Context,
		const TCHAR* Type, const TCHAR*& Buffer,
		const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
};
