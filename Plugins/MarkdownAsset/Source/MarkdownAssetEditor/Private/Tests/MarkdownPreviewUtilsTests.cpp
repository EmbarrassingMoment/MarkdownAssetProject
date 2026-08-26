// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).

#include "Misc/AutomationTest.h"
#include "MarkdownPreviewUtils.h"

#if WITH_DEV_AUTOMATION_TESTS

using MarkdownPreviewUtils::EPreviewUrlAction;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownPreviewSchemeAllowlistTest,
	"MarkdownAsset.Preview.SchemeAllowlist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownPreviewSchemeAllowlistTest::RunTest(const FString& Parameters)
{
	// In-page schemes used by the preview itself.
	TestTrue(TEXT("data: URLs load in-page"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("data:text/html;base64,abc")) == EPreviewUrlAction::AllowInPage);
	TestTrue(TEXT("about:blank loads in-page"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("about:blank")) == EPreviewUrlAction::AllowInPage);

	// Custom asset/class schemes with their decoded payloads.
	FString Payload;
	TestTrue(TEXT("mdasset:// opens a Markdown asset"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("mdasset://My%20Note"), &Payload) == EPreviewUrlAction::OpenMarkdownAsset);
	TestEqual(TEXT("mdasset:// payload is percent-decoded"), Payload, FString(TEXT("My Note")));

	TestTrue(TEXT("ueasset:// opens an Unreal asset"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("ueasset:///Game/Characters/Hero"), &Payload) == EPreviewUrlAction::OpenUnrealAsset);
	TestEqual(TEXT("ueasset:// payload keeps the package path"), Payload, FString(TEXT("/Game/Characters/Hero")));

	TestTrue(TEXT("class:// opens a class"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("class://AActor"), &Payload) == EPreviewUrlAction::OpenClass);
	TestEqual(TEXT("class:// payload is the class name"), Payload, FString(TEXT("AActor")));

	// External URLs always require a user prompt.
	TestTrue(TEXT("https:// prompts before opening externally"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("https://example.com")) == EPreviewUrlAction::PromptExternal);
	TestTrue(TEXT("http:// prompts before opening externally"),
		MarkdownPreviewUtils::ClassifyPreviewUrl(TEXT("http://example.com")) == EPreviewUrlAction::PromptExternal);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownPreviewSchemeDenyTest,
	"MarkdownAsset.Preview.UnknownSchemesBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownPreviewSchemeDenyTest::RunTest(const FString& Parameters)
{
	// Default-deny regression guard: none of these may ever navigate.
	const TCHAR* BlockedUrls[] = {
		TEXT("javascript:alert(1)"),
		TEXT("JaVaScRiPt:alert(1)"),
		TEXT("file:///etc/passwd"),
		TEXT("vbscript:msgbox(1)"),
		TEXT("ftp://example.com/file"),
		TEXT("chrome://settings"),
		TEXT(""),
	};

	for (const TCHAR* Url : BlockedUrls)
	{
		TestTrue(FString::Printf(TEXT("Blocked: '%s'"), Url),
			MarkdownPreviewUtils::ClassifyPreviewUrl(Url) == EPreviewUrlAction::Block);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownPreviewPercentDecodeTest,
	"MarkdownAsset.Preview.PercentDecode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownPreviewPercentDecodeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Plain ASCII passes through"),
		MarkdownPreviewUtils::PercentDecode(TEXT("plain")), FString(TEXT("plain")));

	TestEqual(TEXT("%20 decodes to a space"),
		MarkdownPreviewUtils::PercentDecode(TEXT("My%20Note")), FString(TEXT("My Note")));

	TestEqual(TEXT("Plus decodes to a space"),
		MarkdownPreviewUtils::PercentDecode(TEXT("a+b")), FString(TEXT("a b")));

	TestEqual(TEXT("UTF-8 sequences decode to multibyte characters"),
		MarkdownPreviewUtils::PercentDecode(TEXT("%E3%83%8E%E3%83%BC%E3%83%88")), FString(TEXT("ノート")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarkdownPreviewStyledHtmlTest,
	"MarkdownAsset.Preview.StyledHtmlContentSecurityPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FMarkdownPreviewStyledHtmlTest::RunTest(const FString& Parameters)
{
	const FString Html = MarkdownPreviewUtils::GenerateStyledHtml(TEXT("<p>content</p>"));

	// CSP regression guard: the preview page must keep blocking external
	// requests and script execution.
	TestTrue(TEXT("CSP meta tag is present"),
		Html.Contains(TEXT("Content-Security-Policy")));
	TestTrue(TEXT("CSP defaults to deny-all"),
		Html.Contains(TEXT("default-src 'none'")));
	TestTrue(TEXT("CSP allows inline styles only"),
		Html.Contains(TEXT("style-src 'unsafe-inline'")));
	TestTrue(TEXT("CSP restricts images to data: URIs"),
		Html.Contains(TEXT("img-src data:")));

	TestTrue(TEXT("Parsed content is embedded in the document"),
		Html.Contains(TEXT("<p>content</p>")));

	const int32 CspPos = Html.Find(TEXT("Content-Security-Policy"));
	const int32 BodyPos = Html.Find(TEXT("<body>"));
	TestTrue(TEXT("CSP meta tag appears in the head, before the body"),
		CspPos != INDEX_NONE && BodyPos != INDEX_NONE && CspPos < BodyPos);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
