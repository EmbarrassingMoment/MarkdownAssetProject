# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- **Markitdown Settings (Phase 1)** — Added `UMarkitdownSettings` (Project Settings > Plugins > Markitdown) and `FMarkitdownEnvironment` helpers for executable discovery and command-line composition. Lays the groundwork for the upcoming markitdown-based "Convert to Markdown" import flow (#60); no conversion is performed yet
- **Markitdown Converter (Phase 2)** — Added `FMarkitdownConverter` and `FMarkitdownConversionTask` that invoke markitdown as an external process (synchronous and asynchronous), capture stdout/stderr diagnostics, surface failures via `FMessageDialog` + `UE_LOG`, and expose a `Markitdown.ConvertSync <file>` console command for verification. Import factory / EUW wiring is tracked in #68 / #69 (#67)
- **Markitdown Import Factory (Phase 3)** — Added `UMarkitdownImportFactory` which registers `.pdf` / `.docx` / `.pptx` / `.html` / `.htm` with the Content Browser. Dragging one of these files in (or invoking Reimport on the resulting `UMarkdownAsset`) runs markitdown and stores the converted Markdown as the asset body. EUW batch UI is still tracked in #69 (#68)
- **Markitdown Batch Convert (Phase 4)** — Added `UMarkitdownBlueprintLibrary` exposing `PromptForSourceFiles`, `ConvertFileToMarkdownAsset`, `ConvertFilesToMarkdownAssets`, and `RunBatchConvertWizard` to Blueprint / Editor Utility Widgets, plus a **Tools > Markdown > Batch Convert to Markdown...** menu entry that runs the end-to-end wizard with a cancellable progress dialog and a completion notification (#69)

## [1.2.1] - 2026-04-22

### Fixed

- **Copyright Header** — Replaced leftover Unreal Engine template copyright (`Copyright Epic Games, Inc. All Rights Reserved.`) with the project MIT header (`Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).`) in `Source/MarkdownAssetProject.cpp`, `.h`, `.Build.cs`, and both `.Target.cs` files

## [1.2.0] - 2026-04-18

### Added

- **Asset & Class Links** — Standard Markdown links whose target begins with a UE package root (`/Game/`, `/Engine/`, `/Plugins/`, `/Script/`) are now opened in the corresponding asset editor from the HTML preview
- **Class Link Scheme** — `[Label](class://ClassName)` resolves the target via UClass lookup; native C++ classes open in the IDE via `FSourceCodeNavigation` (preferring the `.cpp` file, falling back to the header), Blueprint classes open in the Blueprint editor
- **Broken Link Styling for New Schemes** — `ueasset://` and `class://` targets that cannot be resolved are highlighted in red in the preview, matching existing wikilink behavior

### Security

- **URL Scheme Allowlist** — The preview's `OnBeforeNavigation` handler now default-denies any scheme that is not explicitly supported (`data:`, `about:`, `mdasset://`, `ueasset://`, `class://`, `http(s)://`); unknown schemes such as `javascript:` and `file:` are blocked to prevent script execution or local file access from untrusted Markdown content
- **Content Security Policy** — Preview HTML now includes a `Content-Security-Policy` meta tag (`default-src 'none'; style-src 'unsafe-inline'; img-src data:`) that blocks all external network requests (remote images, fetch/XHR, frames, fonts), mitigating IP tracking and SSRF against local services from crafted Markdown assets
- **External URL Confirmation Dialog** — Clicking an `http(s)://` link in the preview now presents a localized confirmation dialog showing the full URL before launching the system browser, mitigating phishing via the address-bar-less preview

### Fixed

- **Localization Gather Path** — Corrected `SearchDirectoryPaths` in `Config/Localization/MarkdownEditor_Gather.ini` which pointed to a non-existent directory (`Plugins/MarkdownEditor/Source`) left over from an earlier plugin rename; now targets `Plugins/MarkdownAsset/Source/MarkdownAssetEditor` so `LOCTEXT` / `NSLOCTEXT` strings are collected correctly by the Localization Dashboard

## [1.1.0] - 2026-04-04

### Added

- **Wikilink Support** — Write `[[AssetName]]` in Markdown to create navigable links between Markdown assets
- **Wikilink Navigation** — Clicking a wikilink in the HTML preview opens the target `UMarkdownAsset` in a new editor tab
- **Broken Link Detection** — Wikilinks pointing to non-existent assets are automatically highlighted in red; valid links appear in teal
- **External URL Handling** — Clicking `http://` or `https://` links in the preview opens them in the system browser instead of navigating within the preview pane

### Fixed

- **Line Prefix Insertion** — Fixed toolbar actions (Heading, Bullet List, Numbered List, Blockquote) inserting prefix on wrong line when text wraps visually; now uses absolute offset with backward newline search for correct logical line identification

## [1.0.0] - 2026-03-11

### Added

- **Custom Markdown Asset** — `UMarkdownAsset` as a first-class UObject for storing Markdown text
- **Live HTML Preview** — Dual-pane editor with real-time preview using SWebBrowser (300ms debounce)
- **md4c Integration** — Embedded fast C-based Markdown-to-HTML parser
- **GitHub Flavored Markdown (GFM)** — Support for tables, task lists, and strikethrough
- **Content Browser Integration** — Create Markdown assets via right-click context menu with custom "MD" thumbnails
- **Import/Export** — Drag-and-drop `.md` / `.markdown` file import, reimport support, and export to `.md`
- **Blueprint Support** — Blueprint-callable functions: `GetParsedHTML()`, `GetRawMarkdownText()`, `GetPlainText()`
- **Toolbar & Keyboard Shortcuts** — 12 formatting commands (Bold, Italic, Strikethrough, Code Block, Headings, Lists, Blockquote, Table, Horizontal Rule)
- **Dark Theme** — Styled HTML preview with dark background
- **Undo/Redo** — Full undo/redo support via FScopedTransaction
- **Documentation** — README in English and Japanese
