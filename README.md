# MarkdownAssetProject

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-blue)
![License: MIT](https://img.shields.io/badge/License-MIT-green)
![Platform: Windows](https://img.shields.io/badge/Platform-Windows-lightgrey)
[![Sponsor](https://img.shields.io/badge/Sponsor-EmbarrassingMoment-ff69b4?logo=github-sponsors&logoColor=white)](https://github.com/sponsors/EmbarrassingMoment)

An Unreal Engine 5.5+ plugin that adds a custom Markdown asset type with a live-preview editor.

![Editor Overview](docs/images/editor-overview.png)

## Features

- **Custom Markdown Asset** — `UMarkdownAsset` stores raw Markdown text as a first-class UObject
- **Live HTML Preview** — Dual-pane editor with a text editor on the left and a real-time HTML preview on the right (updates with a 0.3-second debounce for smooth editing)
- **md4c Integration** — Fast Markdown-to-HTML conversion powered by the embedded [md4c](https://github.com/mity/md4c) C library
- **Dark Theme** — Styled HTML output with a dark background for comfortable reading
- **Content Browser Integration** — Create new Markdown assets directly from the context menu, with custom thumbnail previews showing the "MD" label and the first few lines of content
- **Import / Export** — Drag-and-drop `.md` / `.markdown` files into the Content Browser to import, reimport from source files, or export assets back to `.md`
- **GitHub Flavored Markdown** — Supports GFM extensions such as tables, task lists, and strikethrough via the `MD_DIALECT_GITHUB` flag
- **Blueprint Support** — Read/write `RawMarkdownText` and call `GetParsedHTML()`, `GetRawMarkdownText()`, and `GetPlainText()` from Blueprints
- **Toolbar & Keyboard Shortcuts** — Built-in formatting toolbar with keyboard shortcuts for common Markdown operations
- **Undo / Redo** — Full undo/redo support integrated with the Unreal Editor transaction system (Ctrl+Z / Ctrl+Y)
- **Security** — Raw HTML blocks and inline HTML are disabled by default to prevent XSS when rendering user-supplied Markdown
- **Localization** — Editor UI is fully localized for English and Japanese

![Dark Theme Preview](docs/images/dark-theme-preview.png)

![Content Browser Thumbnails](docs/images/content-browser-thumbnail.png)

### Keyboard Shortcuts

| Command | Shortcut |
|---------|----------|
| Bold | Ctrl+B |
| Italic | Ctrl+I |
| Strikethrough | Ctrl+Shift+X |
| Code Block | Ctrl+Shift+C |
| Heading 1 | Ctrl+1 |
| Heading 2 | Ctrl+2 |
| Heading 3 | Ctrl+3 |
| Bullet List | Ctrl+Shift+U |
| Numbered List | Ctrl+Shift+O |
| Blockquote | Ctrl+Shift+Q |
| Insert Table | — |
| Horizontal Rule | — |

![Toolbar Shortcuts](docs/images/toolbar-shortcuts.png)

## Requirements

- Unreal Engine 5.5 or later
- C++ project (the plugin includes native modules)
- `WebBrowserWidget` plugin (enabled automatically as a dependency)

## Installation

1. Clone or copy the `Plugins/MarkdownEditor` directory into your project's `Plugins/` folder.
2. Regenerate project files and build.
3. The plugin will be loaded automatically on editor startup.

## Usage

> **Quick Start:** See [QUICKSTART.md](QUICKSTART.md) for a step-by-step guide.

1. In the Content Browser, right-click and select **Markdown > Markdown Text** to create a new asset.
2. Double-click the asset to open the Markdown editor.
3. Write Markdown in the left pane — the right pane updates the HTML preview in real time.

![Create Asset](docs/images/create-asset.gif)

![Live Preview](docs/images/live-preview.gif)

### Import / Export

- **Import**: Drag a `.md` or `.markdown` file into the Content Browser to create a Markdown asset.
- **Reimport**: Right-click an imported asset and select **Reimport** to reload from the original source file.
- **Export**: Right-click a Markdown asset and select **Asset Actions > Export** to save it as a `.md` file.

### Blueprint Nodes

`UMarkdownAsset` exposes the following Blueprint-callable functions:

| Node | Return Type | Description |
|------|-------------|-------------|
| `GetParsedHTML` | `FString` | Converts the Markdown text to an HTML string using md4c |
| `GetRawMarkdownText` | `FString` | Returns the raw Markdown text as-is |
| `GetPlainText` | `FString` | Returns the text content with all Markdown symbols removed |

- **GetPlainText** is useful for displaying Markdown content in UMG Widgets or 3D text, where Markdown / HTML rendering is not available.
- **GetRawMarkdownText** returns the source Markdown, intended for future extensibility (e.g., custom rendering pipelines).

![Blueprint Nodes](docs/images/blueprint-nodes.png)

## Project Structure

```
Plugins/MarkdownEditor/
├── Source/
│   ├── MarkdownAsset/            # Runtime module
│   │   ├── Public/Private/       # UMarkdownAsset class & md4c wrapper
│   │   └── ThirdParty/md4c/     # Embedded md4c parser library
│   └── MarkdownAssetEditor/      # Editor module
│       └── Public/Private/       # Asset factory, actions, and editor toolkit
└── MarkdownEditor.uplugin
```

| Module | Load Phase | Purpose |
|---|---|---|
| `MarkdownAsset` | Runtime | Core asset class and Markdown-to-HTML conversion |
| `MarkdownAssetEditor` | Editor | Custom asset editor UI with live preview |

## Planned Features

### Editor
- **Syntax Highlighting** — Color-coded Markdown syntax in the text editor
- **Find & Replace** — In-editor text search and replace (Ctrl+F / Ctrl+H)
- **Theme Switching** — Light theme and user-selectable theme options

### Preview
- **Mermaid Diagrams** — Render flowcharts, sequence diagrams, and more via Mermaid
- **Image Preview** — Display referenced images in the HTML preview pane

### Asset Pipeline
- **PDF / HTML Export** — Export Markdown assets as PDF or standalone HTML files

### Platform
- **Multi-platform Support** — macOS and Linux compatibility

### Runtime
- **UMG Markdown Widget** — A UMG widget for rendering Markdown directly in game UI

## FAQ

**Q. Does this plugin work in packaged games?**
A. Yes. While the Markdown Editor and live preview are editor-only features, the `UMarkdownAsset` and its Blueprint-callable functions (like `GetParsedHTML`) are part of the runtime module and work perfectly in packaged builds.

**Q. Do I need an internet connection to parse Markdown?**
A. No. The plugin uses the lightweight md4c C library statically linked into the module. All Markdown-to-HTML conversion is done locally and instantly on your machine.

**Q. How do I display the Markdown content in my game's UI?**
A. You have a few options:
1. Use the `GetParsedHTML` Blueprint node and feed the result into a WebBrowser UMG widget for fully styled text.
2. Use the `GetPlainText` node to strip all Markdown formatting and display it in a standard UMG Text Block.

**Q. Can I edit my .md files in an external editor like VSCode?**
A. Yes. You can import any `.md` or `.markdown` file into the Content Browser. If you edit the original file externally, simply right-click the asset in Unreal Engine and select **Reimport** to update it.

**Q. Is the WebBrowserWidget plugin required?**
A. Yes. The Engine's built-in WebBrowserWidget plugin is required to render the live HTML preview in the custom editor. This plugin will automatically enable it for you.

## License

This project is licensed under the [MIT License](LICENSE).
