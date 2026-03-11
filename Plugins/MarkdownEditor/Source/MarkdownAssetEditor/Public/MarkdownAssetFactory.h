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
	/** Initializes the factory to create new Markdown assets from the Content Browser. */
	UMarkdownAssetFactory();

	/** Creates a new empty UMarkdownAsset instance. */
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
