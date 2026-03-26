# CLAUDE.md

## プロジェクト概要

MarkdownAssetProject は Unreal Engine 5.5+ 用プラグイン「MarkdownAsset」の開発プロジェクトです。

- カスタム Markdown アセットタイプ（`UMarkdownAsset`）とライブプレビュー付きエディタを提供
- md4c ライブラリによる高速な Markdown → HTML 変換
- GitHub Flavored Markdown（GFM）対応（テーブル、タスクリスト、取り消し線）
- Content Browser 統合、Blueprint サポート、日英ローカライズ対応
- ライセンス: MIT / 作者: Kurorekishi (EmbarrassingMoment)

## ライセンスヘッダー

新規 `.cpp` / `.h` ファイルを作成する際は、ファイルの先頭行に以下のライセンスヘッダーを必ず付与すること：

```cpp
// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).
```

## 禁止事項

- `Plugins/MarkdownAsset/Source/MarkdownAsset/ThirdParty/md4c/` 配下のファイルを編集してはならない（サードパーティ製 Markdown パーサー）
- プロジェクトのライセンス（MIT）を変更してはならない
