// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarkdownEditorSettings.generated.h"

/** Color theme used by the HTML preview and the standalone HTML export. */
UENUM(BlueprintType)
enum class EMarkdownPreviewTheme : uint8
{
	Dark UMETA(DisplayName = "Dark"),
	Light UMETA(DisplayName = "Light")
};

/**
 * Per-user preferences for the Markdown asset editor.
 * Surfaced under Editor Preferences > Plugins > Markdown Editor.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Markdown Editor"))
class MARKDOWNASSETEDITOR_API UMarkdownEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMarkdownEditorSettings();

	/** Broadcast after any property on this settings object changes. */
	DECLARE_MULTICAST_DELEGATE(FOnMarkdownEditorSettingsChanged);
	FOnMarkdownEditorSettingsChanged OnSettingsChanged;

	/** Places the page under Editor Preferences rather than Project Settings. */
	virtual FName GetContainerName() const override;
	virtual FName GetCategoryName() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Color theme of the HTML preview pane. */
	UPROPERTY(EditAnywhere, config, Category = "Preview")
	EMarkdownPreviewTheme PreviewTheme;

	/** Delay in seconds between the last keystroke and the preview / outline refresh. */
	UPROPERTY(EditAnywhere, config, Category = "Preview", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "2.0"))
	float PreviewUpdateDelay;

	/** Font size of the Markdown text editor pane. */
	UPROPERTY(EditAnywhere, config, Category = "Editor", meta = (ClampMin = "6", ClampMax = "48", UIMin = "8", UIMax = "32"))
	int32 EditorFontSize;

	/** Content Browser folder that receives texture assets created by Paste Image (e.g. /Game/Markdown/Images). */
	UPROPERTY(EditAnywhere, config, Category = "Images", meta = (ContentDir))
	FDirectoryPath PastedImageFolder;

	/** Returns the pasted-image folder as a long package path with no trailing slash, falling back to the default when unset. */
	FString GetPastedImagePackagePath() const;

	/** Returns the theme to apply to the standalone HTML export (currently the preview theme). */
	EMarkdownPreviewTheme GetExportTheme() const { return PreviewTheme; }
};
