// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Exporters/Exporter.h"
#include "MarkdownEditorSettings.h"
#include "MarkdownAssetHtmlExporter.generated.h"

class UMarkdownAsset;

/**
 * Exporter that writes a UMarkdownAsset as a standalone .html file: the parsed
 * Markdown wrapped in the preview's themed CSS, with referenced project
 * textures embedded as data: URIs so the file has no external dependencies.
 * Available from Asset Actions > Export next to the .md exporter.
 */
UCLASS()
class MARKDOWNASSETEDITOR_API UMarkdownAssetHtmlExporter : public UExporter
{
	GENERATED_BODY()

public:
	UMarkdownAssetHtmlExporter();

	/** Writes the standalone HTML document as UTF-8 to the output archive. */
	virtual bool ExportBinary(UObject* Object, const TCHAR* Type, FArchive& Ar, FFeedbackContext* Warn, int32 FileIndex, uint32 PortFlags) override;

	/**
	 * Builds the standalone HTML document for a Markdown asset: md4c output with
	 * broken-link marking, heading anchors, embedded textures and the themed CSS.
	 * The Content-Security-Policy of the in-editor preview is omitted so that
	 * ordinary external images render when the file is opened in a browser.
	 */
	static FString BuildStandaloneHtml(const UMarkdownAsset* MarkdownAsset, EMarkdownPreviewTheme Theme);
};
