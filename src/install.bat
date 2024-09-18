@echo off
chcp 932

set APP_NAME=SDPlugin
set APP_DIR=%USERPROFILE%\Documents\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\

echo ------------------------------------------------------------
echo プラグインをCLIP Studioのプラグイン用フォルダにインストールします。
echo アンインストールする時はフォルダごと削除してください。
echo インストール先：  %APP_DIR%
echo ------------------------------------------------------------
pause

echo フォルダ作成...
mkdir %APP_DIR%

echo プラグインをコピー...
copy /Y SDPlugin.cpm %APP_DIR%

echo 設定ファイルをコピー...
copy /Y SDPlugin.ini %APP_DIR%

echo DLLをコピー...
copy /Y stable-diffusion.dll %APP_DIR%

echo ------------------------------------------------------------
echo インストール完了
echo ------------------------------------------------------------
pause
