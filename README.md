# MarkdownAssetProject

An Unreal Engine 5.5 plugin that adds a custom Markdown asset type with a live-preview editor.

## Features

- **Custom Markdown Asset** — `UMarkdownAsset` stores raw Markdown text as a first-class UObject
- **Live HTML Preview** — Dual-pane editor with a text editor on the left and a real-time HTML preview on the right
- **md4c Integration** — Fast Markdown-to-HTML conversion powered by the embedded [md4c](https://github.com/mity/md4c) C library
- **Dark Theme** — Styled HTML output with a dark background for comfortable reading
- **Content Browser Integration** — Create new Markdown assets directly from the context menu
- **Import / Export** — Drag-and-drop `.md` / `.markdown` files into the Content Browser to import, reimport from source files, or export assets back to `.md`
- **GitHub Flavored Markdown** — Supports GFM extensions such as tables, task lists, and strikethrough via the `MD_DIALECT_GITHUB` flag
- **Blueprint Support** — Read/write `RawMarkdownText` and call `GetParsedHTML()` from Blueprints

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

### Import / Export

- **Import**: Drag a `.md` or `.markdown` file into the Content Browser to create a Markdown asset.
- **Reimport**: Right-click an imported asset and select **Reimport** to reload from the original source file.
- **Export**: Right-click a Markdown asset and select **Asset Actions > Export** to save it as a `.md` file.

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
