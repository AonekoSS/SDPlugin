# SDPlugin
クリスタのフィルターとして動作する画像生成プラグイン。
生成のバックエンドは [stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) を使用してるので理論上はFluxもいける筈。

まだテスト段階。

## TODO:
- プロパティとか画面の要素整理
- 詳細設定をiniファイルに外部化する
- ControlNetのテスト（anytestとか使えたら最強やん）

## メモ

- プロンプト欄の挙動がおかしい時がある（入力内容が表示されなかったり反映されなかったりする）
- まだアルファチャンネルや選択領域のマスクを無視してます。不透明なレイヤー上で生成してください。
- プラグイン入れたフォルダに「debuglog.txt」という名前でデバッグログ吐いてます（生成経過も出してるからちょっと多いかも）
- 選択範囲のサイズで生成します。デカいと死にます。
- 動作モード：TXT2IMG→プロンプトだけから生成、IMG2IMG→画面上の絵でi2i、CONTROL→画面上の絵をCNに渡す
- 設定ファイルは「COMMON」セクションを読み込んだ後に各モードの設定でオーバーライドします

## 使用方法

以下のパスにフォルダごと放り込んでください。
「ドキュメント/CELSYS/CLIPStudioModule/PlugIn/PAINT」

以下の３ファイルが同じところに入ってる状態にしてください。
・SDPlugin.cpm
・SDPlugin.ini
・stable-diffusion.dll

「SDPlugin.ini」ファイル内のモデルのパスとかを調整してください。
最低限、model_pathさえ設定されてれば動くはず。
