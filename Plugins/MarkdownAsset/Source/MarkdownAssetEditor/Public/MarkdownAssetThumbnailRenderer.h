// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "MarkdownAssetThumbnailRenderer.generated.h"

/**
 * Renders a simple text-based thumbnail for Markdown assets in the Content Browser.
 */
UCLASS()
class UMarkdownAssetThumbnailRenderer : public UThumbnailRenderer
{
	GENERATED_BODY()

public:
	/** Returns the fixed thumbnail dimensions (256x256). */
	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;

	/** Draws the thumbnail with a dark background, "MD" label, and a preview of the Markdown content. */
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;

	/** Returns true if the given object is a UMarkdownAsset that can be visualized. */
	virtual bool CanVisualizeAsset(UObject* Object) override;
};
