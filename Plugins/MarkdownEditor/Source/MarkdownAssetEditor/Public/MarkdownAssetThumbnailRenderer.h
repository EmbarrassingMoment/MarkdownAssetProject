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
	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;
	virtual bool CanVisualizeAsset(UObject* Object) override;
};
