// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkitdownConverter.h"

#include "MarkitdownEnvironment.h"
#include "MarkitdownSettings.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "ISettingsModule.h"
#include "Logging/LogMacros.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/MonitoredProcess.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FMarkitdownConverter"

DEFINE_LOG_CATEGORY_STATIC(LogMarkitdown, Log, All);

namespace
{
	FString MakeTempOutputPath()
	{
		return FPaths::CreateTempFilename(*FPaths::ProjectIntermediateDir(), TEXT("MarkitdownOut_"), TEXT(".md"));
	}

	FText FormatMissingExecutableError(const UMarkitdownSettings& Settings)
	{
		switch (Settings.ExecutionMode)
		{
		case EMarkitdownExecutionMode::Uvx:
			return LOCTEXT("MarkitdownMissingUvx",
				"Could not locate 'uvx' on PATH. Install uv (https://github.com/astral-sh/uv) or change Execution Mode in Project Settings > Plugins > Markitdown.");

		case EMarkitdownExecutionMode::SystemPython:
			return LOCTEXT("MarkitdownMissingPython",
				"Could not locate the Python executable. Set Python Executable Path in Project Settings > Plugins > Markitdown.");

		case EMarkitdownExecutionMode::Custom:
			return LOCTEXT("MarkitdownMissingCustom",
				"Custom command is empty or invalid. Configure it in Project Settings > Plugins > Markitdown.");
		}
		return LOCTEXT("MarkitdownMissingUnknown", "Markitdown executable could not be resolved.");
	}
}

bool FMarkitdownConverter::Preflight(FText& OutError)
{
	const UMarkitdownSettings* Settings = GetDefault<UMarkitdownSettings>();
	if (!Settings)
	{
		OutError = LOCTEXT("MarkitdownNoSettings", "Markitdown settings are not available.");
		return false;
	}

	const FString Executable = FMarkitdownEnvironment::ResolveExecutable(*Settings);
	if (Executable.IsEmpty())
	{
		OutError = FormatMissingExecutableError(*Settings);
		return false;
	}
	return true;
}

void FMarkitdownConverter::OpenSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	const UMarkitdownSettings* Settings = GetDefault<UMarkitdownSettings>();
	if (SettingsModule && Settings)
	{
		SettingsModule->ShowViewer(Settings->GetContainerName(), Settings->GetCategoryName(), Settings->GetSectionName());
	}
}

void FMarkitdownConverter::ShowMissingExecutableDialog()
{
	FText Error;
	if (Preflight(Error))
	{
		return;
	}

	const FText Title = LOCTEXT("MarkitdownDialogTitle", "Markitdown");
	const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::OkCancel, Error, Title);
	if (Result == EAppReturnType::Ok)
	{
		OpenSettings();
	}
}

TSharedPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> FMarkitdownConverter::ConvertAsync(const FString& InputFilePath, FOnMarkitdownConversionComplete OnComplete)
{
	TSharedPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> Task = MakeShared<FMarkitdownConversionTask, ESPMode::ThreadSafe>();
	if (!Task->Start(InputFilePath, OnComplete))
	{
		return nullptr;
	}
	return Task;
}

FMarkitdownConversionResult FMarkitdownConverter::ConvertSync(const FString& InputFilePath)
{
	FMarkitdownConversionResult Result;

	FText PreflightError;
	if (!Preflight(PreflightError))
	{
		Result.ErrorMessage = PreflightError;
		UE_LOG(LogMarkitdown, Warning, TEXT("Preflight failed: %s"), *PreflightError.ToString());
		return Result;
	}

	if (!FPaths::FileExists(InputFilePath))
	{
		Result.ErrorMessage = FText::Format(LOCTEXT("MarkitdownInputMissing", "Input file does not exist: {0}"), FText::FromString(InputFilePath));
		UE_LOG(LogMarkitdown, Warning, TEXT("%s"), *Result.ErrorMessage.ToString());
		return Result;
	}

	const UMarkitdownSettings* Settings = GetDefault<UMarkitdownSettings>();
	const FString Executable = FMarkitdownEnvironment::ResolveExecutable(*Settings);
	const FString TempOutput = MakeTempOutputPath();
	const FString Args = FMarkitdownEnvironment::BuildArguments(*Settings, InputFilePath, TempOutput);

	UE_LOG(LogMarkitdown, Log, TEXT("Running (sync): %s %s"), *Executable, *Args);

	FString StdOut;
	FString StdErr;
	int32 ReturnCode = -1;
	const bool bRan = FPlatformProcess::ExecProcess(*Executable, *Args, &ReturnCode, &StdOut, &StdErr);

	Result.ExitCode = ReturnCode;
	Result.Diagnostics = FString::Printf(TEXT("STDOUT:\n%s\nSTDERR:\n%s"), *StdOut, *StdErr);

	if (!bRan)
	{
		Result.ErrorMessage = LOCTEXT("MarkitdownExecFailed", "Failed to launch markitdown process.");
		UE_LOG(LogMarkitdown, Error, TEXT("ExecProcess failed for %s"), *Executable);
		IFileManager::Get().Delete(*TempOutput, false, true, true);
		return Result;
	}

	if (ReturnCode != 0)
	{
		Result.ErrorMessage = FText::Format(LOCTEXT("MarkitdownNonZero", "markitdown exited with code {0}."), ReturnCode);
		UE_LOG(LogMarkitdown, Error, TEXT("markitdown exit %d\n%s"), ReturnCode, *Result.Diagnostics);
		IFileManager::Get().Delete(*TempOutput, false, true, true);
		return Result;
	}

	if (!FFileHelper::LoadFileToString(Result.OutputMarkdown, *TempOutput))
	{
		Result.ErrorMessage = LOCTEXT("MarkitdownNoOutput", "markitdown completed but its output file could not be read.");
		UE_LOG(LogMarkitdown, Error, TEXT("Output file missing: %s"), *TempOutput);
		IFileManager::Get().Delete(*TempOutput, false, true, true);
		return Result;
	}

	IFileManager::Get().Delete(*TempOutput, false, true, true);
	Result.bSucceeded = true;
	UE_LOG(LogMarkitdown, Log, TEXT("Sync conversion succeeded (%d chars)"), Result.OutputMarkdown.Len());
	return Result;
}

FMarkitdownConversionTask::FMarkitdownConversionTask() = default;

FMarkitdownConversionTask::~FMarkitdownConversionTask()
{
	if (!TempOutputPath.IsEmpty())
	{
		IFileManager::Get().Delete(*TempOutputPath, false, true, true);
	}
}

bool FMarkitdownConversionTask::Start(const FString& InputFilePath, FOnMarkitdownConversionComplete OnComplete)
{
	Completion = OnComplete;

	FText PreflightError;
	if (!FMarkitdownConverter::Preflight(PreflightError))
	{
		FMarkitdownConversionResult Result;
		Result.ErrorMessage = PreflightError;
		UE_LOG(LogMarkitdown, Warning, TEXT("Preflight failed: %s"), *PreflightError.ToString());
		DeliverResult(MoveTemp(Result));
		return false;
	}

	if (!FPaths::FileExists(InputFilePath))
	{
		FMarkitdownConversionResult Result;
		Result.ErrorMessage = FText::Format(LOCTEXT("MarkitdownInputMissing", "Input file does not exist: {0}"), FText::FromString(InputFilePath));
		UE_LOG(LogMarkitdown, Warning, TEXT("%s"), *Result.ErrorMessage.ToString());
		DeliverResult(MoveTemp(Result));
		return false;
	}

	const UMarkitdownSettings* Settings = GetDefault<UMarkitdownSettings>();
	const FString Executable = FMarkitdownEnvironment::ResolveExecutable(*Settings);
	TempOutputPath = MakeTempOutputPath();
	const FString Args = FMarkitdownEnvironment::BuildArguments(*Settings, InputFilePath, TempOutputPath);

	UE_LOG(LogMarkitdown, Log, TEXT("Running (async): %s %s"), *Executable, *Args);

	Process = MakeShared<FMonitoredProcess>(Executable, Args, /*bHidden=*/true);

	TWeakPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> WeakSelf = AsShared();

	Process->OnOutput().BindLambda([WeakSelf](FString Output)
	{
		if (TSharedPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> Self = WeakSelf.Pin())
		{
			Self->HandleOutput(MoveTemp(Output));
		}
	});

	Process->OnCompleted().BindLambda([WeakSelf](int32 ReturnCode)
	{
		if (TSharedPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> Self = WeakSelf.Pin())
		{
			Self->HandleCompleted(ReturnCode);
		}
	});

	if (!Process->Launch())
	{
		FMarkitdownConversionResult Result;
		Result.ErrorMessage = LOCTEXT("MarkitdownExecFailed", "Failed to launch markitdown process.");
		UE_LOG(LogMarkitdown, Error, TEXT("FMonitoredProcess::Launch returned false"));
		Process.Reset();
		DeliverResult(MoveTemp(Result));
		return false;
	}

	return true;
}

void FMarkitdownConversionTask::Cancel()
{
	if (Process.IsValid())
	{
		Process->Cancel(true);
	}
}

void FMarkitdownConversionTask::HandleOutput(FString Output)
{
	OutputBuffer += Output;
	OutputBuffer += TEXT("\n");
}

void FMarkitdownConversionTask::HandleCompleted(int32 ReturnCode)
{
	FMarkitdownConversionResult Result;
	Result.ExitCode = ReturnCode;
	Result.Diagnostics = OutputBuffer;

	if (ReturnCode == 0)
	{
		if (FFileHelper::LoadFileToString(Result.OutputMarkdown, *TempOutputPath))
		{
			Result.bSucceeded = true;
		}
		else
		{
			Result.ErrorMessage = LOCTEXT("MarkitdownNoOutput", "markitdown completed but its output file could not be read.");
			UE_LOG(LogMarkitdown, Error, TEXT("Output file missing: %s"), *TempOutputPath);
		}
	}
	else
	{
		Result.ErrorMessage = FText::Format(LOCTEXT("MarkitdownNonZero", "markitdown exited with code {0}."), ReturnCode);
		UE_LOG(LogMarkitdown, Error, TEXT("markitdown exit %d\n%s"), ReturnCode, *Result.Diagnostics);
	}

	if (!TempOutputPath.IsEmpty())
	{
		IFileManager::Get().Delete(*TempOutputPath, false, true, true);
		TempOutputPath.Empty();
	}

	DeliverResult(MoveTemp(Result));
}

void FMarkitdownConversionTask::DeliverResult(FMarkitdownConversionResult Result)
{
	if (bDelivered.AtomicSet(true))
	{
		return;
	}

	FOnMarkitdownConversionComplete LocalDelegate = Completion;

	if (IsInGameThread())
	{
		LocalDelegate.ExecuteIfBound(Result);
		return;
	}

	AsyncTask(ENamedThreads::GameThread, [LocalDelegate, ResultCopy = MoveTemp(Result)]() mutable
	{
		LocalDelegate.ExecuteIfBound(ResultCopy);
	});
}

namespace
{
	void ExecuteConsoleConvertSync(const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogMarkitdown, Warning, TEXT("Usage: Markitdown.ConvertSync <input-file-path>"));
			return;
		}

		const FString InputPath = FPaths::ConvertRelativePathToFull(Args[0]);
		const FMarkitdownConversionResult Result = FMarkitdownConverter::ConvertSync(InputPath);
		if (!Result.bSucceeded)
		{
			UE_LOG(LogMarkitdown, Error, TEXT("Conversion failed: %s"), *Result.ErrorMessage.ToString());
			UE_LOG(LogMarkitdown, Error, TEXT("Diagnostics:\n%s"), *Result.Diagnostics);
			return;
		}

		const int32 PreviewLen = FMath::Min(Result.OutputMarkdown.Len(), 500);
		UE_LOG(LogMarkitdown, Display, TEXT("Conversion succeeded (%d chars). First %d chars:\n%s"),
			Result.OutputMarkdown.Len(), PreviewLen, *Result.OutputMarkdown.Left(PreviewLen));
	}

	FAutoConsoleCommand GMarkitdownConvertSyncCmd(
		TEXT("Markitdown.ConvertSync"),
		TEXT("Synchronously convert <file> via markitdown and log the result."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&ExecuteConsoleConvertSync));
}

#undef LOCTEXT_NAMESPACE
