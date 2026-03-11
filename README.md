# MarkdownAssetProject

An Unreal Engine 5.5 plugin that adds a custom Markdown asset type with a live-preview editor.

<!-- TODO: Insert image - Main screenshot of the dual-pane Markdown editor (left: Markdown source, right: HTML preview) -->
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

<!-- TODO: Insert image - Dark theme HTML preview pane showing styled Markdown output -->
![Dark Theme Preview](docs/images/dark-theme-preview.png)

<!-- TODO: Insert image - Content Browser showing Markdown asset thumbnails with "MD" label and content preview -->
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

<!-- TODO: Insert image - Close-up of the formatting toolbar buttons (Bold, Italic, Heading, etc.) -->
![Toolbar Shortcuts](docs/images/toolbar-shortcuts.png)

## Requirements

- Unreal Engine 5.5
- C++ project (the plugin includes native modules)
- `WebBrowserWidget` plugin (enabled automatically as a dependency)

## Installation

1. Clone or copy the `Plugins/MarkdownEditor` directory into your project's `Plugins/` folder.
2. Regenerate project files and build.
3. The plugin will be loaded automatically on editor startup.

## Usage

1. In the Content Browser, right-click and select **Miscellaneous > Markdown Text** to create a new asset.
2. Double-click the asset to open the Markdown editor.
3. Write Markdown in the left pane — the right pane updates the HTML preview in real time.

<!-- TODO: Insert GIF - Screen recording of creating a new Markdown asset via Content Browser right-click menu -->
![Create Asset](docs/images/create-asset.gif)

<!-- TODO: Insert GIF - Screen recording of typing Markdown and seeing the HTML preview update in real time -->
![Live Preview](docs/images/live-preview.gif)

### Import / Export

- **Import**: Drag a `.md` or `.markdown` file into the Content Browser to create a Markdown asset.
- **Reimport**: Right-click an imported asset and select **Reimport** to reload from the original source file.
- **Export**: Right-click a Markdown asset and select **Asset Actions > Export** to save it as a `.md` file.

<!-- TODO: Insert GIF - Screen recording of drag-and-drop import and export menu operation -->
![Import and Export](docs/images/import-export.gif)

### Blueprint Nodes

`UMarkdownAsset` exposes the following Blueprint-callable functions:

| Node | Return Type | Description |
|------|-------------|-------------|
| `GetParsedHTML` | `FString` | Converts the Markdown text to an HTML string using md4c |
| `GetRawMarkdownText` | `FString` | Returns the raw Markdown text as-is |
| `GetPlainText` | `FString` | Returns the text content with all Markdown symbols removed |

- **GetPlainText** is useful for displaying Markdown content in UMG Widgets or 3D text, where Markdown / HTML rendering is not available.
- **GetRawMarkdownText** returns the source Markdown, intended for future extensibility (e.g., custom rendering pipelines).

<!-- TODO: Insert image - Screenshot of Blueprint editor showing GetParsedHTML, GetRawMarkdownText, and GetPlainText nodes -->
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

## License

This project is licensed under the [MIT License](LICENSE).
