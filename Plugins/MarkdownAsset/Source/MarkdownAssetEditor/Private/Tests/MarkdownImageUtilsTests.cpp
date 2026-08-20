// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownImageUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownImageEmbedPassthroughTest,
	"MarkdownAsset.Images.NonPackageSourcesUntouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownImageEmbedPassthroughTest::RunTest(const FString& Parameters)
{
	const FString ExternalImg = TEXT("<p><img src=\"https://example.com/a.png\" alt=\"a\"></p>");
	TestEqual(TEXT("External image src is left untouched"),
		MarkdownImageUtils::EmbedTextureImages(ExternalImg), ExternalImg);

	const FString DataImg = TEXT("<img src=\"data:image/png;base64,AAAA\" alt=\"inline\">");
	TestEqual(TEXT("data: image src is left untouched"),
		MarkdownImageUtils::EmbedTextureImages(DataImg), DataImg);

	const FString NoImages = TEXT("<h1>Title</h1><p>text</p>");
	TestEqual(TEXT("HTML without images is unchanged"),
		MarkdownImageUtils::EmbedTextureImages(NoImages), NoImages);

	TestEqual(TEXT("Empty HTML stays empty"),
		MarkdownImageUtils::EmbedTextureImages(FString()), FString());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownImageEmbedMissingTextureTest,
	"MarkdownAsset.Images.MissingTexturePlaceholder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownImageEmbedMissingTextureTest::RunTest(const FString& Parameters)
{
	const FString Result = MarkdownImageUtils::EmbedTextureImages(
		TEXT("<p><img src=\"/Game/MarkdownTests/DoesNotExist_XyZ\" alt=\"x\"></p>"));

	TestTrue(TEXT("Missing texture renders the placeholder span"),
		Result.Contains(TEXT("md-missing-image")));
	TestTrue(TEXT("Placeholder names the missing path"),
		Result.Contains(TEXT("/Game/MarkdownTests/DoesNotExist_XyZ")));
	TestFalse(TEXT("Missing texture does not leave an <img> tag"),
		Result.Contains(TEXT("<img")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
