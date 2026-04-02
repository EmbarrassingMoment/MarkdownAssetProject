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
		const FString& RawText = MarkdownAsset->RawMarkdownText;
		int32 TextLen = FMath::Min(RawText.Len(), 200);

		float LineY = Y + 40.0f;
		int32 LineCount = 0;
		int32 CurrentPos = 0;

		while (LineCount < 8 && CurrentPos < TextLen)
		{
			int32 NextNewlinePos = -1;
			int32 SubStrLen = TextLen - CurrentPos;

			// Find next newline up to 200 character limit
			for (int32 i = CurrentPos; i < TextLen; ++i)
			{
				if (RawText[i] == TEXT('\n') || RawText[i] == TEXT('\r'))
				{
					NextNewlinePos = i;
					SubStrLen = i - CurrentPos;
					break;
				}
			}

			// ParseIntoArrayLines culls empty strings by default
			if (SubStrLen > 0)
			{
				int32 DrawLen = FMath::Min(SubStrLen, 40);
				FString Line = RawText.Mid(CurrentPos, DrawLen);
				FCanvasTextItem TextItem(FVector2D(X + 6, LineY), FText::FromString(Line), SmallFont, FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
				TextItem.Scale = FVector2D(0.8f, 0.8f);
				Canvas->DrawItem(TextItem);
				LineY += 14.0f;
				LineCount++;
			}

			if (NextNewlinePos != -1)
			{
				CurrentPos = NextNewlinePos + 1;
				// Handle \r\n
				if (CurrentPos < TextLen && RawText[NextNewlinePos] == TEXT('\r') && RawText[CurrentPos] == TEXT('\n'))
				{
					CurrentPos++;
				}
			}
			else
			{
				break;
			}
		}
	}
}

/** Returns true if the object is a valid UMarkdownAsset instance. */
bool UMarkdownAssetThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	return Object && Object->IsA<UMarkdownAsset>();
}
