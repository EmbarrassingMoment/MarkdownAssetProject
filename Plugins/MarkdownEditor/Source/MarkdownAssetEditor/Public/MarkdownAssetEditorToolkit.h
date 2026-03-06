// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UMarkdownAsset;
class SMultiLineEditableTextBox;
class SWebBrowser;

class FMarkdownAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	void Initialize(UMarkdownAsset* InMarkdownAsset, const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost);

	// IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

private:
	void OnTextChanged(const FText& NewText);
	TSharedRef<SDockTab> SpawnTab_Main(const FSpawnTabArgs& Args);

	UMarkdownAsset* MarkdownAsset;

	TSharedPtr<SMultiLineEditableTextBox> EditableTextBox;
	TSharedPtr<SWebBrowser> WebBrowserWidget;

	static const FName AppIdentifier;
	static const FName MainTabId;
};
