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
