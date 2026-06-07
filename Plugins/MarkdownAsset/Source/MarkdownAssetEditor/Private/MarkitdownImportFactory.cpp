// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "MarkitdownImportFactory.h"

#include "MarkdownAsset.h"
#include "MarkitdownConverter.h"

#include "Misc/FeedbackContext.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "UMarkitdownImportFactory"

namespace
{
	bool IsSupportedExtension(const FString& Extension)
	{
		const FString Lower = Extension.ToLower();
		return Lower == TEXT("pdf")
			|| Lower == TEXT("docx")
			|| Lower == TEXT("pptx")
			|| Lower == TEXT("html")
			|| Lower == TEXT("htm");
	}

	void ShowImportFailureDialog(const FString& Filename, const FText& Reason)
	{
		const FText Message = FText::Format(
			LOCTEXT("MarkitdownImportFailed", "Failed to convert {0} via markitdown:\n\n{1}"),
			FText::FromString(FPaths::GetCleanFilename(Filename)),
			Reason);
		FMessageDialog::Open(EAppMsgType::Ok, Message, LOCTEXT("MarkitdownImportTitle", "Markitdown Import"));
	}
}

UMarkitdownImportFactory::UMarkitdownImportFactory()
{
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true;
	bText = false;
	SupportedClass = UMarkdownAsset::StaticClass();

	Formats.Add(TEXT("pdf;Portable Document Format"));
	Formats.Add(TEXT("docx;Microsoft Word Document"));
	Formats.Add(TEXT("pptx;Microsoft PowerPoint Presentation"));
	Formats.Add(TEXT("html;HTML Document"));
	Formats.Add(TEXT("htm;HTML Document"));
}

bool UMarkitdownImportFactory::FactoryCanImport(const FString& Filename)
{
	return IsSupportedExtension(FPaths::GetExtension(Filename));
}

UObject* UMarkitdownImportFactory::FactoryCreateFile(
	UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, const FString& Filename, const TCHAR* /*Parms*/,
	FFeedbackContext* /*Warn*/, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	FText PreflightError;
	if (!FMarkitdownConverter::Preflight(PreflightError))
	{
		FMarkitdownConverter::ShowMissingExecutableDialog();
		return nullptr;
	}

	FScopedSlowTask SlowTask(1.0f,
		FText::Format(LOCTEXT("MarkitdownConverting", "Converting {0} via markitdown..."),
			FText::FromString(FPaths::GetCleanFilename(Filename))));
	SlowTask.MakeDialog();

	const FMarkitdownConversionResult Result = FMarkitdownConverter::ConvertSync(Filename);
	SlowTask.EnterProgressFrame(1.0f);

	if (!Result.bSucceeded)
	{
		ShowImportFailureDialog(Filename, Result.ErrorMessage);
		return nullptr;
	}

	UMarkdownAsset* NewAsset = NewObject<UMarkdownAsset>(InParent, InClass, InName, Flags | RF_Transactional);
	if (NewAsset)
	{
		NewAsset->RawMarkdownText = Result.OutputMarkdown;
#if WITH_EDITORONLY_DATA
		NewAsset->SourceFilePath = Filename;
#endif
	}
	return NewAsset;
}

bool UMarkitdownImportFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UMarkdownAsset* Asset = Cast<UMarkdownAsset>(Obj);
	if (!Asset)
	{
		return false;
	}
#if WITH_EDITORONLY_DATA
	const FString& SourcePath = Asset->SourceFilePath;
	if (SourcePath.IsEmpty() || !IsSupportedExtension(FPaths::GetExtension(SourcePath)))
	{
		return false;
	}
	OutFilenames.Add(SourcePath);
	return true;
#else
	return false;
#endif
}

void UMarkitdownImportFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UMarkdownAsset* Asset = Cast<UMarkdownAsset>(Obj);
	if (Asset && ensure(NewReimportPaths.Num() == 1))
	{
#if WITH_EDITORONLY_DATA
		Asset->SourceFilePath = NewReimportPaths[0];
#endif
	}
}

EReimportResult::Type UMarkitdownImportFactory::Reimport(UObject* Obj)
{
	UMarkdownAsset* Asset = Cast<UMarkdownAsset>(Obj);
	if (!Asset)
	{
		return EReimportResult::Failed;
	}

#if WITH_EDITORONLY_DATA
	const FString Filename = Asset->SourceFilePath;
	if (Filename.IsEmpty() || !FPaths::FileExists(Filename))
	{
		return EReimportResult::Failed;
	}

	FText PreflightError;
	if (!FMarkitdownConverter::Preflight(PreflightError))
	{
		FMarkitdownConverter::ShowMissingExecutableDialog();
		return EReimportResult::Failed;
	}

	FScopedSlowTask SlowTask(1.0f,
		FText::Format(LOCTEXT("MarkitdownReimporting", "Re-converting {0} via markitdown..."),
			FText::FromString(FPaths::GetCleanFilename(Filename))));
	SlowTask.MakeDialog();

	const FMarkitdownConversionResult Result = FMarkitdownConverter::ConvertSync(Filename);
	SlowTask.EnterProgressFrame(1.0f);

	if (!Result.bSucceeded)
	{
		ShowImportFailureDialog(Filename, Result.ErrorMessage);
		return EReimportResult::Failed;
	}

	Asset->RawMarkdownText = Result.OutputMarkdown;
	Asset->MarkPackageDirty();
	return EReimportResult::Succeeded;
#else
	return EReimportResult::Failed;
#endif
}

int32 UMarkitdownImportFactory::GetPriority() const
{
	return ImportPriority;
}

#undef LOCTEXT_NAMESPACE
