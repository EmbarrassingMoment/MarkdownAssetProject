// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkitdownSettings.h"

UMarkitdownSettings::UMarkitdownSettings()
	: ExecutionMode(EMarkitdownExecutionMode::Uvx)
{
}

FName UMarkitdownSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

#if WITH_EDITOR
void UMarkitdownSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Edits made through Project Settings > Plugins > Markitdown are not reliably
	// flushed to disk on their own, so the executable path / custom command were
	// lost on editor restart. Persist them to DefaultEditor.ini explicitly.
	TryUpdateDefaultConfigFile();
}
#endif
