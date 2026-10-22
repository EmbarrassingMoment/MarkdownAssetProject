// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownEditorSettings.h"

namespace
{
	const TCHAR* DefaultPastedImageFolder = TEXT("/Game/Markdown/Images");
}

UMarkdownEditorSettings::UMarkdownEditorSettings()
	: PreviewTheme(EMarkdownPreviewTheme::Dark)
	, PreviewUpdateDelay(0.3f)
	, EditorFontSize(12)
{
	PastedImageFolder.Path = DefaultPastedImageFolder;
}

FName UMarkdownEditorSettings::GetContainerName() const
{
	return TEXT("Editor");
}

FName UMarkdownEditorSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

#if WITH_EDITOR
void UMarkdownEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SaveConfig();
	OnSettingsChanged.Broadcast();
}
#endif

FString UMarkdownEditorSettings::GetPastedImagePackagePath() const
{
	FString Path = PastedImageFolder.Path;
	Path.TrimStartAndEndInline();

	// Accept both "/Game/Foo" and "Game/Foo" style input and normalise it.
	Path.ReplaceInline(TEXT("\\"), TEXT("/"));
	while (Path.EndsWith(TEXT("/")))
	{
		Path.LeftChopInline(1);
	}
	if (!Path.IsEmpty() && !Path.StartsWith(TEXT("/")))
	{
		Path = TEXT("/") + Path;
	}

	if (Path.IsEmpty() || !Path.StartsWith(TEXT("/Game/")))
	{
		return DefaultPastedImageFolder;
	}

	return Path;
}
