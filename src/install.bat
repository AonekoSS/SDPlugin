@echo off
chcp 932

set APP_NAME=SDPlugin
set APP_DIR=%USERPROFILE%\Documents\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\

echo ------------------------------------------------------------
echo �v���O�C����CLIP Studio�̃v���O�C���p�t�H���_�ɃC���X�g�[�����܂��B
echo �A���C���X�g�[�����鎞�̓t�H���_���ƍ폜���Ă��������B
echo �C���X�g�[����F  %APP_DIR%
echo ------------------------------------------------------------
pause

echo �t�H���_�쐬...
mkdir %APP_DIR%

echo �v���O�C�����R�s�[...
copy /Y SDPlugin.cpm %APP_DIR%

echo �ݒ�t�@�C�����R�s�[...
copy /Y SDPlugin.ini %APP_DIR%

echo DLL���R�s�[...
copy /Y stable-diffusion.dll %APP_DIR%

echo ------------------------------------------------------------
echo �C���X�g�[������
echo ------------------------------------------------------------
pause
