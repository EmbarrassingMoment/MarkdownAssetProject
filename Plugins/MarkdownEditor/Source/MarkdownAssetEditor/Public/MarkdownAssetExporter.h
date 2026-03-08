// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "MarkdownAssetExporter.generated.h"

/**
 * Exporter for exporting UMarkdownAsset as a .md file.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkdownAssetExporter : public UExporter
{
	GENERATED_BODY()

public:
	UMarkdownAssetExporter();

	// UExporter interface
	virtual bool ExportText(const FExportObjectInnerContext* Context, UObject* Object, const TCHAR* Type, FOutputDevice& Ar, FFeedbackContext* Warn, uint32 PortFlags) override;
};
