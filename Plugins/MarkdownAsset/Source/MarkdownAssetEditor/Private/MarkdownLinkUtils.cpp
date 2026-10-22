// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkdownLinkUtils.h"
#include "MarkdownAsset.h"
#include "MarkdownPreviewUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Internationalization/Regex.h"
#include "Misc/PackageName.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

/**
 * Builds reflection-name candidates to try when resolving a user-supplied class name.
 * UHT strips the leading A/U/I prefix from native class names, so "class://AActor"
 * must be matched against the UClass named "Actor". Conversely, a bare "Actor" may
 * need to be resolved as "AActor" for a hypothetical class in some projects.
 */
TArray<FString> MarkdownLinkUtils::BuildClassNameCandidates(const FString& ClassName)
{
	TArray<FString> Candidates;
	if (ClassName.IsEmpty())
	{
		return Candidates;
	}

	Candidates.Add(ClassName);

	// Strip a single-letter A/U/I prefix when followed by an uppercase letter.
	if (ClassName.Len() > 1 && FChar::IsUpper(ClassName[1]))
	{
		const TCHAR First = ClassName[0];
		if (First == TEXT('A') || First == TEXT('U') || First == TEXT('I'))
		{
			Candidates.AddUnique(ClassName.Mid(1));
		}
	}

	// Add A/U-prefixed variants for bare names.
	if (FChar::IsUpper(ClassName[0]))
	{
		Candidates.AddUnique(FString::Printf(TEXT("A%s"), *ClassName));
		Candidates.AddUnique(FString::Printf(TEXT("U%s"), *ClassName));
	}

	return Candidates;
}

/** Returns true if an Unreal asset exists at the given object path. */
bool MarkdownLinkUtils::DoesAssetExistAtPath(const FString& ObjectPath)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Strip a trailing ".ObjectName" to derive the package path; the registry indexes by package.
	FString PackagePath = ObjectPath;
	int32 DotIndex;
	if (PackagePath.FindChar(TEXT('.'), DotIndex))
	{
		PackagePath.LeftInline(DotIndex);
	}

	TArray<FAssetData> PackageAssets;
	AssetRegistry.GetAssetsByPackageName(FName(*PackagePath), PackageAssets);
	if (PackageAssets.Num() > 0)
	{
		return true;
	}

	// Secondary: legacy object-path query in case the caller supplied a non-package form.
	if (AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath)).IsValid())
	{
		return true;
	}

	// Last resort: does the package file exist on disk? Catches assets the registry has not
	// yet indexed (e.g. newly added plugin content).
	return FPackageName::DoesPackageExist(PackagePath);
}

/** Returns true if a native UClass or Blueprint class matching ClassName can be resolved. */
bool MarkdownLinkUtils::DoesClassExist(const FString& ClassName)
{
	if (ClassName.IsEmpty())
	{
		return false;
	}

	// Native class lookup against reflection names (UHT strips A/U/I prefixes).
	for (const FString& Candidate : BuildClassNameCandidates(ClassName))
	{
		if (FindFirstObject<UClass>(*Candidate, EFindFirstObjectOptions::NativeFirst) != nullptr)
		{
			return true;
		}
	}

	// Blueprint fallback via Asset Registry (accepts both "BP_Foo" and "BP_Foo_C").
	FString BlueprintAssetName = ClassName;
	if (BlueprintAssetName.EndsWith(TEXT("_C")))
	{
		BlueprintAssetName.LeftChopInline(2);
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> BlueprintAssets;
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
	for (const FAssetData& Asset : BlueprintAssets)
	{
		if (Asset.AssetName.ToString() == BlueprintAssetName)
		{
			return true;
		}
	}

	return false;
}

/**
 * Walks every <a href="..."> tag and adds the "md-broken-link" CSS class when
 * the target of an mdasset://, ueasset://, or class:// scheme cannot be resolved.
 * External URLs and other schemes are left untouched.
 */
FString MarkdownLinkUtils::MarkBrokenLinks(const FString& Html)
{
	// Cache MarkdownAsset names once for mdasset:// lookups.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AllMarkdownAssets;
	AssetRegistry.GetAssetsByClass(UMarkdownAsset::StaticClass()->GetClassPathName(), AllMarkdownAssets);

	TSet<FString> MarkdownAssetNames;
	for (const FAssetData& Asset : AllMarkdownAssets)
	{
		MarkdownAssetNames.Add(Asset.AssetName.ToString());
	}

	const FRegexPattern LinkPattern(TEXT("<a href=\"(mdasset|ueasset|class)://([^\"]*)\""));
	FRegexMatcher Matcher(LinkPattern, Html);

	FString Result;
	int32 LastPos = 0;

	while (Matcher.FindNext())
	{
		Result += Html.Mid(LastPos, Matcher.GetMatchBeginning() - LastPos);

		const FString Scheme = Matcher.GetCaptureGroup(1);
		const FString EncodedTarget = Matcher.GetCaptureGroup(2);
		const FString DecodedTarget = MarkdownPreviewUtils::PercentDecode(EncodedTarget);

		bool bTargetExists = false;
		if (Scheme == TEXT("mdasset"))
		{
			bTargetExists = MarkdownAssetNames.Contains(DecodedTarget);
		}
		else if (Scheme == TEXT("ueasset"))
		{
			bTargetExists = DoesAssetExistAtPath(DecodedTarget);
		}
		else // class
		{
			bTargetExists = DoesClassExist(DecodedTarget);
		}

		if (bTargetExists)
		{
			Result += Html.Mid(Matcher.GetMatchBeginning(), Matcher.GetMatchEnding() - Matcher.GetMatchBeginning());
		}
		else
		{
			Result += FString::Printf(TEXT("<a class=\"md-broken-link\" href=\"%s://%s\""), *Scheme, *EncodedTarget);
		}

		LastPos = Matcher.GetMatchEnding();
	}
	Result += Html.Mid(LastPos);

	return Result;
}

FString MarkdownLinkUtils::BuildMarkdownLinkForAsset(const FString& PackageName, const FString& AssetName, const UClass* AssetClass)
{
	if (AssetClass && AssetClass->IsChildOf(UMarkdownAsset::StaticClass()))
	{
		return FString::Printf(TEXT("[[%s]]"), *AssetName);
	}

	if (AssetClass && AssetClass->IsChildOf(UTexture2D::StaticClass()))
	{
		return FString::Printf(TEXT("![%s](%s)"), *AssetName, *PackageName);
	}

	return FString::Printf(TEXT("[%s](%s)"), *AssetName, *PackageName);
}
