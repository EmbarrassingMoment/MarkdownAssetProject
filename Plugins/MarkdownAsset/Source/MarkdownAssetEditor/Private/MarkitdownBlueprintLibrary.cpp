// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkitdownBlueprintLibrary.h"

#include "MarkdownAsset.h"
#include "MarkitdownConverter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAssetTools.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "UMarkitdownBlueprintLibrary"

namespace
{
	const void* GetActiveWindowHandle()
	{
		if (FSlateApplication::IsInitialized())
		{
			const TSharedPtr<SWindow> ActiveWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
			if (ActiveWindow.IsValid() && ActiveWindow->GetNativeWindow().IsValid())
			{
				return ActiveWindow->GetNativeWindow()->GetOSWindowHandle();
			}
		}
		return nullptr;
	}

	void ShowCompletionNotification(int32 NumSucceeded, int32 NumRequested, const FString& OutPackagePath)
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("BatchConvertComplete", "Markitdown: {0} / {1} asset(s) created in {2}"),
			FText::AsNumber(NumSucceeded),
			FText::AsNumber(NumRequested),
			FText::FromString(OutPackagePath)));
		Info.ExpireDuration = 5.0f;
		Info.bUseSuccessFailIcons = true;
		const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(NumSucceeded > 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		}
	}
}

TArray<FString> UMarkitdownBlueprintLibrary::PromptForSourceFiles()
{
	TArray<FString> OutFiles;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return OutFiles;
	}

	const FString Filter = TEXT("Supported files (*.pdf;*.docx;*.pptx;*.html;*.htm)|*.pdf;*.docx;*.pptx;*.html;*.htm|All files (*.*)|*.*");
	DesktopPlatform->OpenFileDialog(
		GetActiveWindowHandle(),
		LOCTEXT("PickSourceFilesTitle", "Select files to convert via markitdown").ToString(),
		FPaths::ProjectDir(),
		TEXT(""),
		Filter,
		EFileDialogFlags::Multiple,
		OutFiles);

	for (FString& Path : OutFiles)
	{
		Path = FPaths::ConvertRelativePathToFull(Path);
	}
	return OutFiles;
}

UMarkdownAsset* UMarkitdownBlueprintLibrary::ConvertFileToMarkdownAsset(const FString& InputFilePath, const FString& OutPackagePath, FString AssetName)
{
	if (OutPackagePath.IsEmpty())
	{
		return nullptr;
	}

	const FMarkitdownConversionResult Result = FMarkitdownConverter::ConvertSync(InputFilePath);
	if (!Result.bSucceeded)
	{
		return nullptr;
	}

	if (AssetName.IsEmpty())
	{
		AssetName = FPaths::GetBaseFilename(InputFilePath);
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetToolsModule.Get().CreateUniqueAssetName(OutPackagePath / AssetName, FString(), UniquePackageName, UniqueAssetName);

	UPackage* Package = CreatePackage(*UniquePackageName);
	if (!Package)
	{
		return nullptr;
	}

	UMarkdownAsset* NewAsset = NewObject<UMarkdownAsset>(Package, FName(*UniqueAssetName), RF_Public | RF_Standalone | RF_Transactional);
	if (!NewAsset)
	{
		return nullptr;
	}

	NewAsset->RawMarkdownText = Result.OutputMarkdown;
#if WITH_EDITORONLY_DATA
	NewAsset->SourceFilePath = InputFilePath;
#endif

	FAssetRegistryModule::AssetCreated(NewAsset);
	Package->MarkPackageDirty();
	return NewAsset;
}

TArray<UMarkdownAsset*> UMarkitdownBlueprintLibrary::ConvertFilesToMarkdownAssets(const TArray<FString>& InputFilePaths, const FString& OutPackagePath)
{
	TArray<UMarkdownAsset*> Created;
	if (InputFilePaths.Num() == 0 || OutPackagePath.IsEmpty())
	{
		return Created;
	}

	FText PreflightError;
	if (!FMarkitdownConverter::Preflight(PreflightError))
	{
		FMarkitdownConverter::ShowMissingExecutableDialog();
		return Created;
	}

	FScopedSlowTask SlowTask(static_cast<float>(InputFilePaths.Num()),
		LOCTEXT("BatchConverting", "Converting files via markitdown..."));
	SlowTask.MakeDialog(/*bShowCancelButton=*/true);

	for (const FString& Path : InputFilePaths)
	{
		if (SlowTask.ShouldCancel())
		{
			break;
		}

		SlowTask.EnterProgressFrame(1.0f,
			FText::Format(LOCTEXT("BatchConvertingItem", "Converting {0}..."),
				FText::FromString(FPaths::GetCleanFilename(Path))));

		if (UMarkdownAsset* Asset = ConvertFileToMarkdownAsset(Path, OutPackagePath, FString()))
		{
			Created.Add(Asset);
		}
	}

	return Created;
}

void UMarkitdownBlueprintLibrary::RunBatchConvertWizard(const FString& OutPackagePath)
{
	const FString TargetPath = OutPackagePath.IsEmpty() ? TEXT("/Game/Markitdown") : OutPackagePath;

	const TArray<FString> Files = PromptForSourceFiles();
	if (Files.Num() == 0)
	{
		return;
	}

	const TArray<UMarkdownAsset*> Created = ConvertFilesToMarkdownAssets(Files, TargetPath);
	ShowCompletionNotification(Created.Num(), Files.Num(), TargetPath);
}

#undef LOCTEXT_NAMESPACE
