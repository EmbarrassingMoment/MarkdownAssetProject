# Quick Start Guide

Get up and running with the Markdown Editor plugin in minutes.

## Prerequisites

- Unreal Engine 5.5
- A C++ project

## 1. Install the Plugin

1. Copy the `Plugins/MarkdownEditor` directory into your project's `Plugins/` folder.
2. Regenerate project files and build.
3. Launch the editor — the plugin loads automatically.

## 2. Create Your First Markdown Asset

1. In the **Content Browser**, right-click and select **Miscellaneous > Markdown Text**.
2. Double-click the new asset to open the editor.
3. Write Markdown in the left pane — the right pane shows a live HTML preview.

<!-- TODO: Insert GIF - Screen recording of creating a Markdown asset from the Content Browser right-click menu -->
![Create Asset](docs/images/create-asset.gif)

<!-- TODO: Insert GIF - Screen recording of live Markdown editing with real-time HTML preview -->
![Live Preview](docs/images/live-preview.gif)

## 3. Import & Export

- **Import** — Drag a `.md` or `.markdown` file into the Content Browser.
- **Reimport** — Right-click an imported asset > **Reimport**.
- **Export** — Right-click an asset > **Asset Actions > Export** to save as `.md`.

<!-- TODO: Insert GIF - Screen recording of drag-and-drop import and export workflow -->
![Import and Export](docs/images/import-export.gif)

## 4. Use in Blueprints

`UMarkdownAsset` provides three Blueprint-callable functions:

| Node | Description |
|------|-------------|
| `GetParsedHTML` | Converts Markdown to HTML |
| `GetRawMarkdownText` | Returns the raw Markdown source |
| `GetPlainText` | Returns text with Markdown symbols stripped |

<!-- TODO: Insert image - Screenshot of Blueprint editor showing the three Markdown asset nodes -->
![Blueprint Nodes](docs/images/blueprint-nodes.png)

## Key Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+B | Bold |
| Ctrl+I | Italic |
| Ctrl+Shift+X | Strikethrough |
| Ctrl+Shift+C | Code Block |
| Ctrl+1 / 2 / 3 | Heading 1 / 2 / 3 |
| Ctrl+Shift+U | Bullet List |
| Ctrl+Shift+O | Numbered List |
| Ctrl+Shift+Q | Blockquote |

## Next Steps

See [README.md](README.md) for the full documentation, including project structure and advanced features.
