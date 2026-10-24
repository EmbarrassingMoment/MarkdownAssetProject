// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownAsset.h"
#include "MarkdownLinkUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownLinkForDroppedAssetTest,
	"MarkdownAsset.Links.MarkdownLinkForDroppedAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownLinkForDroppedAssetTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Markdown assets become wikilinks"),
		MarkdownLinkUtils::BuildMarkdownLinkForAsset(TEXT("/Game/Docs/Design"), TEXT("Design"), UMarkdownAsset::StaticClass()),
		FString(TEXT("[[Design]]")));

	TestEqual(TEXT("Textures become image links"),
		MarkdownLinkUtils::BuildMarkdownLinkForAsset(TEXT("/Game/UI/T_Icon"), TEXT("T_Icon"), UTexture2D::StaticClass()),
		FString(TEXT("![T_Icon](/Game/UI/T_Icon)")));

	TestEqual(TEXT("Blueprints become asset links"),
		MarkdownLinkUtils::BuildMarkdownLinkForAsset(TEXT("/Game/BP/BP_Door"), TEXT("BP_Door"), UBlueprint::StaticClass()),
		FString(TEXT("[BP_Door](/Game/BP/BP_Door)")));

	TestEqual(TEXT("Unknown class falls back to an asset link"),
		MarkdownLinkUtils::BuildMarkdownLinkForAsset(TEXT("/Game/Misc/Thing"), TEXT("Thing"), nullptr),
		FString(TEXT("[Thing](/Game/Misc/Thing)")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownLinkClassNameCandidatesTest,
	"MarkdownAsset.Links.ClassNameCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownLinkClassNameCandidatesTest::RunTest(const FString& Parameters)
{
	const TArray<FString> FromPrefixed = MarkdownLinkUtils::BuildClassNameCandidates(TEXT("AActor"));
	TestTrue(TEXT("Prefixed name is kept"), FromPrefixed.Contains(TEXT("AActor")));
	TestTrue(TEXT("Prefix is stripped for reflection lookup"), FromPrefixed.Contains(TEXT("Actor")));

	const TArray<FString> FromBare = MarkdownLinkUtils::BuildClassNameCandidates(TEXT("Actor"));
	TestTrue(TEXT("Bare name gains the A prefix"), FromBare.Contains(TEXT("AActor")));
	TestTrue(TEXT("Bare name gains the U prefix"), FromBare.Contains(TEXT("UActor")));

	TestEqual(TEXT("Empty name yields no candidates"),
		MarkdownLinkUtils::BuildClassNameCandidates(TEXT("")).Num(), 0);

	TestTrue(TEXT("Native engine class resolves"), MarkdownLinkUtils::DoesClassExist(TEXT("AActor")));
	TestFalse(TEXT("Unknown class does not resolve"), MarkdownLinkUtils::DoesClassExist(TEXT("ThisClassDoesNotExist_XyZ")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
