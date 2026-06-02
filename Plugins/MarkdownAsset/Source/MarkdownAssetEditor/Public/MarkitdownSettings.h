// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarkitdownSettings.generated.h"

/** How the markitdown CLI is invoked. */
UENUM(BlueprintType)
enum class EMarkitdownExecutionMode : uint8
{
	Uvx UMETA(DisplayName = "uvx (recommended)"),
	SystemPython UMETA(DisplayName = "System Python (python -m markitdown)"),
	Custom UMETA(DisplayName = "Custom command")
};

/**
 * Project settings for the markitdown integration.
 * Surfaced under Project Settings > Plugins > Markitdown.
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Markitdown"))
class MARKDOWNASSETEDITOR_API UMarkitdownSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMarkitdownSettings();

	virtual FName GetCategoryName() const override;

	/** Strategy used to invoke the markitdown tool. */
	UPROPERTY(EditAnywhere, config, Category = "Execution")
	EMarkitdownExecutionMode ExecutionMode;

	/** Path to the Python executable. Used when ExecutionMode is SystemPython. */
	UPROPERTY(EditAnywhere, config, Category = "Execution",
		meta = (EditCondition = "ExecutionMode == EMarkitdownExecutionMode::SystemPython"))
	FFilePath PythonExecutablePath;

	/** Full command line used when ExecutionMode is Custom (e.g. "C:/tools/markitdown.exe"). */
	UPROPERTY(EditAnywhere, config, Category = "Execution",
		meta = (EditCondition = "ExecutionMode == EMarkitdownExecutionMode::Custom"))
	FString CustomCommand;

	/** Extra command-line arguments appended to every markitdown invocation. */
	UPROPERTY(EditAnywhere, config, Category = "Execution")
	FString ExtraArguments;
};
