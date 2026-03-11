// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownAssetThumbnailRenderer.h"
#include "MarkdownAsset.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"

/** Returns fixed 256x256 thumbnail dimensions. */
void UMarkdownAssetThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	OutWidth = 256;
	OutHeight = 256;
}

/** Draws a dark-themed thumbnail with an "MD" label and a preview of the first few lines of content. */
void UMarkdownAssetThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UMarkdownAsset* MarkdownAsset = Cast<UMarkdownAsset>(Object);
	if (!MarkdownAsset || !Canvas)
	{
		return;
	}

	// Draw dark background
	Canvas->DrawTile(X, Y, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor(0.12f, 0.12f, 0.12f, 1.0f));

	// Draw "MD" label at top
	UFont* Font = GEngine ? GEngine->GetLargeFont() : nullptr;
	if (Font)
	{
		FCanvasTextItem LabelItem(FVector2D(X + 8, Y + 4), FText::FromString(TEXT("MD")), Font, FLinearColor(0.22f, 0.58f, 1.0f, 1.0f));
		LabelItem.Scale = FVector2D(1.5f, 1.5f);
		Canvas->DrawItem(LabelItem);
	}

	// Draw preview of markdown content
	UFont* SmallFont = GEngine ? GEngine->GetSmallFont() : nullptr;
	if (SmallFont && !MarkdownAsset->RawMarkdownText.IsEmpty())
	{
		FString PreviewText = MarkdownAsset->RawMarkdownText.Left(200);

		// Split into lines and draw first few
		TArray<FString> Lines;
		PreviewText.ParseIntoArrayLines(Lines);

		float LineY = Y + 40.0f;
		int32 MaxLines = FMath::Min(Lines.Num(), 8);
		for (int32 i = 0; i < MaxLines; i++)
		{
			FString Line = Lines[i].Left(40);
			FCanvasTextItem TextItem(FVector2D(X + 6, LineY), FText::FromString(Line), SmallFont, FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
			TextItem.Scale = FVector2D(0.8f, 0.8f);
			Canvas->DrawItem(TextItem);
			LineY += 14.0f;
		}
	}
}

/** Returns true if the object is a valid UMarkdownAsset instance. */
bool UMarkdownAssetThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return Object && Object->IsA<UMarkdownAsset>();
}
