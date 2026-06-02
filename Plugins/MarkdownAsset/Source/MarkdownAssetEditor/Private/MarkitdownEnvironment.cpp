// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkitdownEnvironment.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"

namespace
{
	FString QuoteIfNeeded(const FString& Path)
	{
		if (Path.IsEmpty() || (Path.StartsWith(TEXT("\"")) && Path.EndsWith(TEXT("\""))))
		{
			return Path;
		}
		if (Path.Contains(TEXT(" ")))
		{
			return FString::Printf(TEXT("\"%s\""), *Path);
		}
		return Path;
	}
}

FString FMarkitdownEnvironment::FindExecutableInPath(const FString& ExecutableName)
{
	if (ExecutableName.IsEmpty())
	{
		return FString();
	}

	const FString PathEnv = FPlatformMisc::GetEnvironmentVariable(TEXT("PATH"));
	if (PathEnv.IsEmpty())
	{
		return FString();
	}

#if PLATFORM_WINDOWS
	const TCHAR PathSeparator = TEXT(';');
#else
	const TCHAR PathSeparator = TEXT(':');
#endif

	TArray<FString> Directories;
	PathEnv.ParseIntoArray(Directories, &PathSeparator, /*InCullEmpty=*/true);

	TArray<FString> CandidateNames;
	CandidateNames.Add(ExecutableName);
#if PLATFORM_WINDOWS
	if (!ExecutableName.EndsWith(TEXT(".exe"), ESearchCase::IgnoreCase)
		&& !ExecutableName.EndsWith(TEXT(".cmd"), ESearchCase::IgnoreCase)
		&& !ExecutableName.EndsWith(TEXT(".bat"), ESearchCase::IgnoreCase))
	{
		CandidateNames.Add(ExecutableName + TEXT(".exe"));
		CandidateNames.Add(ExecutableName + TEXT(".cmd"));
	}
#endif

	IFileManager& FileManager = IFileManager::Get();
	for (const FString& Dir : Directories)
	{
		for (const FString& Name : CandidateNames)
		{
			const FString Candidate = FPaths::Combine(Dir, Name);
			if (FileManager.FileExists(*Candidate))
			{
				return FPaths::ConvertRelativePathToFull(Candidate);
			}
		}
	}

	return FString();
}

FString FMarkitdownEnvironment::ResolveExecutable(const UMarkitdownSettings& Settings)
{
	switch (Settings.ExecutionMode)
	{
	case EMarkitdownExecutionMode::Uvx:
		return FindExecutableInPath(TEXT("uvx"));

	case EMarkitdownExecutionMode::SystemPython:
	{
		const FString Configured = Settings.PythonExecutablePath.FilePath;
		if (!Configured.IsEmpty() && IFileManager::Get().FileExists(*Configured))
		{
			return FPaths::ConvertRelativePathToFull(Configured);
		}
		return FindExecutableInPath(TEXT("python"));
	}

	case EMarkitdownExecutionMode::Custom:
		return Settings.CustomCommand.TrimStartAndEnd();
	}

	return FString();
}

FString FMarkitdownEnvironment::BuildArguments(const UMarkitdownSettings& Settings, const FString& InputPath, const FString& OutputPath)
{
	const FString QuotedInput = QuoteIfNeeded(InputPath);
	const FString QuotedOutput = QuoteIfNeeded(OutputPath);

	FString Args;
	switch (Settings.ExecutionMode)
	{
	case EMarkitdownExecutionMode::Uvx:
		Args = FString::Printf(TEXT("markitdown %s -o %s"), *QuotedInput, *QuotedOutput);
		break;

	case EMarkitdownExecutionMode::SystemPython:
		Args = FString::Printf(TEXT("-m markitdown %s -o %s"), *QuotedInput, *QuotedOutput);
		break;

	case EMarkitdownExecutionMode::Custom:
		Args = FString::Printf(TEXT("%s -o %s"), *QuotedInput, *QuotedOutput);
		break;
	}

	const FString Extra = Settings.ExtraArguments.TrimStartAndEnd();
	if (!Extra.IsEmpty())
	{
		Args += TEXT(" ");
		Args += Extra;
	}
	return Args;
}
