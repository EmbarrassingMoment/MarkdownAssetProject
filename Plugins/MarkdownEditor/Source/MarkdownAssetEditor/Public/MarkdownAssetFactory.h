// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MarkdownAssetFactory.generated.h"

/**
 * Factory for creating UMarkdownAsset objects.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkdownAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMarkdownAssetFactory();

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
};
