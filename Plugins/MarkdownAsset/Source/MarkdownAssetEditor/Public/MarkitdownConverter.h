// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Templates/SharedPointer.h"

class FMonitoredProcess;

/** Outcome of a markitdown conversion. */
struct MARKDOWNASSETEDITOR_API FMarkitdownConversionResult
{
	bool bSucceeded = false;
	int32 ExitCode = -1;
	FString OutputMarkdown;
	FString Diagnostics;
	FText ErrorMessage;
};

DECLARE_DELEGATE_OneParam(FOnMarkitdownConversionComplete, const FMarkitdownConversionResult& /*Result*/);

/**
 * Handle for an in-flight asynchronous conversion. Keep the returned shared
 * pointer alive until the completion delegate fires (or call Cancel to abort).
 */
class MARKDOWNASSETEDITOR_API FMarkitdownConversionTask : public TSharedFromThis<FMarkitdownConversionTask, ESPMode::ThreadSafe>
{
public:
	FMarkitdownConversionTask();
	~FMarkitdownConversionTask();

	/** Launches the process. Returns false (and synchronously fires Completion with an error) if pre-flight checks fail. */
	bool Start(const FString& InputFilePath, FOnMarkitdownConversionComplete OnComplete);

	/** Requests cancellation of the running process. Completion still fires with bSucceeded=false. */
	void Cancel();

private:
	void HandleOutput(FString Output);
	void HandleCompleted(int32 ReturnCode);
	void DeliverResult(FMarkitdownConversionResult Result);

	TSharedPtr<FMonitoredProcess> Process;
	FString TempOutputPath;
	FString OutputBuffer;
	FOnMarkitdownConversionComplete Completion;
	FThreadSafeBool bDelivered;
};

/**
 * Static facade for invoking markitdown. Resolves the executable from
 * UMarkitdownSettings + FMarkitdownEnvironment and exposes both synchronous
 * and asynchronous entry points. Process invocation only — no UI wiring.
 */
class MARKDOWNASSETEDITOR_API FMarkitdownConverter
{
public:
	/** Validates settings and verifies the executable is locatable. Returns false with a user-facing message otherwise. */
	static bool Preflight(FText& OutError);

	/** Opens Project Settings > Plugins > Markitdown. */
	static void OpenSettings();

	/** Shows a localized OK/Cancel dialog explaining the failure; OK opens the Settings page. */
	static void ShowMissingExecutableDialog();

	/** Launches an asynchronous conversion. Returns null if pre-flight fails (delegate still fires with the error). */
	static TSharedPtr<FMarkitdownConversionTask, ESPMode::ThreadSafe> ConvertAsync(const FString& InputFilePath, FOnMarkitdownConversionComplete OnComplete);

	/** Synchronously converts a file. Blocks the calling thread; use from commandlets / tests / dev console only. */
	static FMarkitdownConversionResult ConvertSync(const FString& InputFilePath);
};
