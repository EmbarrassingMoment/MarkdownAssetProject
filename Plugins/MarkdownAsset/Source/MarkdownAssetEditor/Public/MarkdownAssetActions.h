// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/**
 * Asset type actions for UMarkdownAsset.
 * Defines how Markdown assets appear and behave in the Content Browser,
 * including display name, color, category, and custom editor launching.
 */
class FMarkdownAssetActions : public FAssetTypeActions_Base
{
public:
	/** Constructs the actions with the specified asset category. */
	FMarkdownAssetActions(EAssetTypeCategories::Type InAssetCategory);

	/** Returns the display name shown in the Content Browser context menu. */
	virtual FText GetName() const override;

	/** Returns the color used to represent this asset type in the editor. */
	virtual FColor GetTypeColor() const override;

	/** Returns the UClass that this action supports. */
	virtual UClass* GetSupportedClass() const override;

	/** Returns the asset category bit for filtering in the Content Browser. */
	virtual uint32 GetCategories() override;

	/** Opens the custom Markdown editor for the selected assets. */
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

private:
	/** The asset category this action is registered under. */
	EAssetTypeCategories::Type AssetCategory;
};
