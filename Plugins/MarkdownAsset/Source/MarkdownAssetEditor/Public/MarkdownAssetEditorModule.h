// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FMarkdownAssetActions;

/**
 * Editor module for Markdown assets.
 * Registers asset type actions, custom asset category, and thumbnail renderer
 * so that Markdown assets can be created, edited, and previewed in the Unreal Editor.
 */
class FMarkdownAssetEditorModule : public IModuleInterface
{
public:
	/** Registers asset actions, category, and thumbnail renderer on module startup. */
	virtual void StartupModule() override;

	/** Unregisters asset actions and cleans up on module shutdown. */
	virtual void ShutdownModule() override;

private:
	/** Shared pointer to the registered asset type actions. */
	TSharedPtr<FMarkdownAssetActions> MarkdownAssetActions;

	/** The registered asset category bit for filtering Markdown assets. */
	EAssetTypeCategories::Type MarkdownAssetCategoryBit;
};
