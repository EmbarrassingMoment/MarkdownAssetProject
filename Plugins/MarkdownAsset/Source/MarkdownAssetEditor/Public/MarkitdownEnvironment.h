// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "MarkitdownSettings.h"

/**
 * Stateless helpers for locating the markitdown executable and composing
 * its command line. Process invocation is intentionally out of scope and
 * will be added in a follow-up issue.
 */
class MARKDOWNASSETEDITOR_API FMarkitdownEnvironment
{
public:
	/** Searches the PATH environment variable for ExecutableName. Returns absolute path or empty. */
	static FString FindExecutableInPath(const FString& ExecutableName);

	/** Resolves the executable path to invoke based on the supplied settings. Empty if not found / not configured. */
	static FString ResolveExecutable(const UMarkitdownSettings& Settings);

	/** Builds the argument string passed to the resolved executable for converting InputPath into OutputPath. */
	static FString BuildArguments(const UMarkitdownSettings& Settings, const FString& InputPath, const FString& OutputPath);
};
