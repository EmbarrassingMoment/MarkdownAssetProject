# Feature Tour: v1.5.0

This note exercises everything added in 1.5.0. Import it by dragging the file
into the Content Browser (it becomes a `UMarkdownAsset` named `MD_FeatureTour_1_5`),
open it, and work through the sections in order. The **Outline** tab lists every
heading below, so you can jump between checks.

Checklist:

- [ ] 1. Editor Preferences (theme, delay, font size, paste folder)
- [ ] 2. Light / dark theme rendering
- [ ] 3. Find & Replace
- [ ] 4. Drag & drop links
- [ ] 5. Standalone HTML export
- [ ] 6. Preview texture cache

---

## 1. Editor Preferences

Open **Edit > Editor Preferences > Plugins > Markdown Editor** and keep this
editor visible while you change each value:

| Setting | Try | Expected |
|---------|-----|----------|
| Preview Theme | `Light` | The preview on the right turns white immediately; switch back to `Dark` and it returns |
| Preview Update Delay | `0` then `2.0` | Typing in the editor updates the preview instantly, then only ~2 s after the last keystroke |
| Editor Font Size | `18` | The text on the left grows right away |
| Pasted Image Folder | `/Game/Markdown/TourImages` | **Paste Image** (Ctrl+Shift+V) creates the texture under the new folder |

Reset the values when you are done (defaults: Dark, 0.3, 12, `/Game/Markdown/Images`).

## 2. Theme rendering

Every styled element is below. Compare the two themes: text, headings, code,
tables, quotes, rules, strikethrough, and the four link colors must all stay readable.

Inline `code`, **bold**, *italic*, ~~strikethrough~~, and a [plain https link](https://www.unrealengine.com).

```cpp
// Fenced code block
UMarkdownAsset* Note = NewObject<UMarkdownAsset>();
Note->RawMarkdownText = TEXT("# Hello");
```

> Blockquote with a border on the left.
> It keeps the accent color of the active theme.

| Column A | Column B | Column C |
|----------|----------|----------|
| Row 1    | striped  | rows     |
| Row 2    | alternate| shading  |
| Row 3    | header   | contrast |

- [x] Task list item (done)
- [ ] Task list item (open)

### 2.1 Link colors

- Wikilink, valid: [[MD_Sample]]
- Wikilink, broken (red): [[NoSuchNote_Tour]]
- Asset link, valid: [BP_LinkTest](/Game/BP_LinkTest)
- Asset link, broken (red): [Missing asset](/Game/DoesNotExist_Tour)
- Class link, valid: [AActor](class://AActor)
- Class link, broken (red): [NoSuchClass](class://NoSuchClass_Tour)

### 2.2 Images

- Embedded texture (project asset): ![Circular gauge](/Game/Textures/T_GaugeCircle)
- Missing texture (red placeholder): ![missing](/Game/Textures/T_GaugeNone_Tour)
- External image (blocked by the preview CSP, visible in the HTML export): ![UE badge](https://img.shields.io/badge/Unreal%20Engine-5.5%2B-blue)

## 3. Find & Replace

Press **Ctrl+F**. The bar opens above the editor; if you had a word selected
it is used as the query. The counts below cover the whole document, including
this section's own text.

1. Type `lantern`. The status reads `1 of 7` and the first occurrence is selected.
   Occurrences in this list: lantern, Lantern, LANTERN, lantern-light, lanternlantern (counts twice).
2. Press **Enter** / **F3** repeatedly: the selection moves through every match and wraps to the first.
3. Press **Shift+Enter** / **Shift+F3**: it moves backwards and wraps to the last.
4. Toggle **Aa**: the count drops to the 5 lowercase matches.
5. Type `zzz-not-here`: the status reads `No results`.
6. Press **Ctrl+H** (or the **Replace** toggle). Query `TOUR_TODO`, replacement `done`:
   - **Replace** changes the selected match and jumps to the next one.
   - **Replace All** changes the rest and shows a "Replaced N occurrence(s)." notification (4 in total if you skip the single Replace: this mention plus the three targets).
   - **Ctrl+Z** in the editor restores them.
7. Press **Esc**: the bar closes and focus returns to the editor.

Replace targets: TOUR_TODO one, TOUR_TODO two, TOUR_TODO three.

Multi-line match check (the query `beacon` reports 4: this mention plus the three lines):

    first beacon
    second beacon
    third beacon

## 4. Drag & drop links

Put the cursor on the empty line under each bullet and drop the named asset from
the Content Browser onto the text editor. The tooltip changes to
"Insert a Markdown link to this asset at the cursor" while hovering.

- Drop `MD_Sample` (Markdown asset) → inserts `[[MD_Sample]]` (teal wikilink):

- Drop `T_GaugeSegment` (texture) → inserts `![T_GaugeSegment](/Game/Textures/T_GaugeSegment)` and the image appears in the preview:

- Drop `BP_LinkTest` (Blueprint) → inserts `[BP_LinkTest](/Game/BP_LinkTest)` (yellow asset link):

- Select `MD_Sample` and `T_GaugeCircle` together and drop both → two links, one per line:

## 5. Standalone HTML export

1. Right-click `MD_FeatureTour_1_5` in the Content Browser → **Asset Actions > Export...**
2. Pick **Standalone HTML File (*.html)** in the file type dropdown and save.
3. Open the file in a browser:
   - The page uses the theme selected in Editor Preferences (`<title>` is the asset name).
   - The gauge images from section 2.2 are embedded (no `/Game/` paths, the file works offline).
   - The external badge image **does** load here, unlike in the editor preview.
   - Headings carry `id="md-h-N"` anchors; `file.html#md-h-3` jumps to a section.
   - Non-ASCII text survives as UTF-8: こんにちは、Markdown。
4. Export again as **Markdown File (*.md)** to confirm the original exporter still works.

## 6. Preview texture cache

The two gauge textures above are PNG-encoded once and reused on later preview refreshes.

1. Open **Window > Output Log** and filter by `LogMarkdownImage`.
2. Type a few characters anywhere in this document: the preview refreshes but no new encode warnings appear and the refresh stays fast even with the images present.
3. Open `T_GaugeCircle`, change a setting that touches the source (e.g. reimport from `docs/sample-images/T_GaugeCircle.png`), save, and edit this note again: the preview picks up the new image because the texture's source id changed.
4. Rename or delete `T_GaugeNone_Tour` references above: missing textures are evicted from the cache and render the red placeholder.

---

Done? Everything in the checklist at the top should be ticked.
