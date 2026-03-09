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
};
