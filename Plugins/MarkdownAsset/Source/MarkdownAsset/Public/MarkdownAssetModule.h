// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Runtime module for the Markdown asset type.
 * Registers the MarkdownAsset module so that UMarkdownAsset objects can be used at runtime.
 */
class FMarkdownAssetModule : public IModuleInterface
{
public:

	/** Called when the module is loaded into memory. */
	virtual void StartupModule() override;

	/** Called when the module is unloaded from memory. */
	virtual void ShutdownModule() override;
};
