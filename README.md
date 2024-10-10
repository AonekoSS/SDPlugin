
# SDPlugin - クリスタ用 画像生成プラグイン

クリスタのフィルターとして動作する画像生成プラグインです。


## 注意

このプラグインは開発中のものです。<br>

問い合わせたところオープンソースでの開発が「SDKの使用許諾条件を満たさない」との回答を頂きましたので、公式のSDKを使用せずに開発しています。<br>
その為、プラグインとしての動作に対して保証やサポートをすることはできません。<br>
あくまで自己責任でのご使用をお願いします。<br>


## 使用方法

[リリースページ](https://github.com/AonekoSS/SDPlugin/releases) から最新バージョンをダウンロードしてください。<br>
解凍すると中に「install.bat」というバッチファイルがあるのでそれを実行してください。<br>
プラグイン用のフォルダにファイルがコピーされます。<br>
（分かってる人はSDPluginフォルダ毎、手動で放り込んでも大丈夫です）<br>

インストール先の「SDPlugin.ini」ファイルを環境に合わせて編集してください。<br>
ひとまず「model_path」が使用するモデルのパスになっていれば動くと思います。<br>
（設定の詳細はそのうちどこかに書こうと思います）<br>

クリスタを起動するとメニューの「フィルター(I)」の所に「Stable Diffusion(X)」という項目が増えてるので、そこからご使用ください。<br>


## 開発メモ

- プロンプト欄の挙動がおかしい時があります（入力内容が表示されなかったり反映されなかったりする）
	- 調査中、どうもクリスタのバージョンが古いとなりやすいっぽい？
- 透明なレイヤーには生成しません（生成するけど出力しません）ので適当に塗りつぶしてください
	- レイヤーのアルファチャンネル（不透明度）は維持したま生成します
- 選択領域があるとその範囲にだけ生成します（上手くやるとインペイントっぽい挙動に）
- プラグインを入れたフォルダに「debuglog.txt」という名前でデバッグログを吐いてます
	- 生成経過も出してるからちょっと多いかも？ そのうちオプションで切れるようにします
- 選択範囲のサイズで生成します。デカいと死にます。
- 設定ファイルの内容は "COMMON" セクションが共通で使われ、各セクション毎の設定が優先されます

詳細は [issue](https://github.com/AonekoSS/SDPlugin/issues) を参照ください。<br>


## ライセンス

ソースコードはクレジット無しで自由に弄ってもらって大丈夫です。（The Unlicense）
