// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FMarkdownAssetActions;

class FMarkdownAssetEditorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<FMarkdownAssetActions> MarkdownAssetActions;
	EAssetTypeCategories::Type MarkdownAssetCategoryBit;
};
