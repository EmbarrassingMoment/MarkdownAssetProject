# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-03-11

### Added

- **Custom Markdown Asset** — `UMarkdownAsset` as a first-class UObject for storing Markdown text
- **Live HTML Preview** — Dual-pane editor with real-time preview using SWebBrowser (300ms debounce)
- **md4c Integration** — Embedded fast C-based Markdown-to-HTML parser
- **GitHub Flavored Markdown (GFM)** — Support for tables, task lists, and strikethrough
- **Content Browser Integration** — Create Markdown assets via right-click context menu with custom "MD" thumbnails
- **Import/Export** — Drag-and-drop `.md` file import, reimport support, and export to `.md`
- **Blueprint Support** — Blueprint-callable functions: `GetParsedHTML()`, `GetRawMarkdownText()`, `GetPlainText()`
- **Toolbar & Keyboard Shortcuts** — 12 formatting commands (Bold, Italic, Strikethrough, Code Block, Headings, Lists, Blockquote, Table, Horizontal Rule)
- **Dark Theme** — Styled HTML preview with dark background
- **Undo/Redo** — Full undo/redo support via FScopedTransaction
- **Documentation** — README in English and Japanese
