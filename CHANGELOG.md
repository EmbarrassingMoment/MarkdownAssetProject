# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- **Asset & Class Links** — Standard Markdown links whose target begins with a UE package root (`/Game/`, `/Engine/`, `/Plugins/`, `/Script/`) are now opened in the corresponding asset editor from the HTML preview
- **Class Link Scheme** — `[Label](class://ClassName)` resolves the target via UClass lookup; native C++ classes open in the IDE via `FSourceCodeNavigation::NavigateToClass`, Blueprint classes open in the Blueprint editor
- **Broken Link Styling for New Schemes** — `ueasset://` and `class://` targets that cannot be resolved are highlighted in red in the preview, matching existing wikilink behavior

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
