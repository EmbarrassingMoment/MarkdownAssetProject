# Quick Start Guide

Get up and running with the Markdown Editor plugin in minutes.

## Prerequisites

- Unreal Engine 5.5
- A C++ project
- `WebBrowserWidget` plugin (enabled automatically as a dependency)

## 1. Install the Plugin

1. Copy the `Plugins/MarkdownAsset` directory into your project's `Plugins/` folder.
2. Regenerate project files and build.
3. Launch the editor — the plugin loads automatically.

## 2. Create Your First Markdown Asset

1. In the **Content Browser**, right-click and select **Markdown > Markdown Text**.
2. Double-click the new asset to open the editor.
3. Write Markdown in the left pane — the right pane shows a live HTML preview.

> **Tip:** You can also open the included `MD_Sample` asset in the Content Browser to see an example.

![Create Asset](docs/images/create-asset.gif)

![Live Preview](docs/images/live-preview.gif)

## 3. Import & Export

- **Import** — Drag a `.md` or `.markdown` file into the Content Browser.
- **Reimport** — Right-click an imported asset > **Reimport**.
- **Export** — Right-click an asset > **Asset Actions > Export** to save as `.md`.

> **Importing PDF / DOCX / PPTX / HTML?** These formats are routed through Microsoft's [markitdown](https://github.com/microsoft/markitdown) CLI, which is an optional external dependency. Install it first (`pip install "markitdown[all]"` or `uv`) — see the **Requirements** and **Markitdown** sections in [README.md](README.md) for setup details.

## 4. Use in Blueprints

`UMarkdownAsset` provides three Blueprint-callable functions:

| Node | Description |
|------|-------------|
| `GetParsedHTML` | Converts Markdown to HTML |
| `GetRawMarkdownText` | Returns the raw Markdown source |
| `GetPlainText` | Returns text with Markdown symbols stripped |

![Blueprint Nodes](docs/images/blueprint-nodes.png)

## 5. Link Between Assets with Wikilinks

Use `[[AssetName]]` syntax to create links between Markdown assets:

```markdown
See also: [[MyOtherDocument]]
```

- In the preview, wikilinks appear in **teal** with a dashed underline.
- Click a wikilink to open the target Markdown asset in a new editor tab.
- Links to non-existent assets are shown in **red** so you can spot broken references at a glance.
- Standard Markdown links (`[text](https://...)`) open in your system browser.

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
| — | Insert Table (toolbar only) |
| — | Horizontal Rule (toolbar only) |

## Next Steps

See [README.md](README.md) for the full documentation, including project structure and advanced features.
