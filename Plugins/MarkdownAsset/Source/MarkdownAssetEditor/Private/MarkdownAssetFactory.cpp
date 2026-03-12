// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetFactory.h"
#include "MarkdownAsset.h"

/** Configures the factory to create new Markdown assets via the Content Browser "Add" menu. */
UMarkdownAssetFactory::UMarkdownAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
	SupportedClass = UMarkdownAsset::StaticClass();
}

/** Creates and returns a new empty UMarkdownAsset with transactional support. */
UObject* UMarkdownAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
}
