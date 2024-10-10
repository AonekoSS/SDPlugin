@echo off
chcp 932

set APP_NAME=SDPlugin

:RESTART
echo ------------------------------------------------------------
echo プラグインのインストール先は、CLIP Studioのバージョンによって違います。
echo お使いのバージョンが「1.10.13」以降であれば「1」を、それより古い場合は「2」を入力してください。
echo ------------------------------------------------------------
set /p VER="バージョン確認: "
IF %VER% == 1 (
	set APP_DIR=%USERPROFILE%\AppData\Roaming\CELSYSUserData\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\
) ELSE IF %VER% == 2 (
	set APP_DIR=%USERPROFILE%\Documents\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\
) ELSE (
	GOTO RESTART
)

echo ------------------------------------------------------------
echo プラグインをプラグイン用フォルダにインストールします。
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
