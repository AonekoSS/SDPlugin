@echo off
chcp 932

set APP_NAME=SDPlugin

:RESTART
echo ------------------------------------------------------------
echo �v���O�C���̃C���X�g�[����́ACLIP Studio�̃o�[�W�����ɂ���ĈႢ�܂��B
echo ���g���̃o�[�W�������u1.10.13�v�ȍ~�ł���΁u1�v���A������Â��ꍇ�́u2�v����͂��Ă��������B
echo ------------------------------------------------------------
set /p VER="�o�[�W�����m�F: "
IF %VER% == 1 (
	set APP_DIR=%USERPROFILE%\AppData\Roaming\CELSYSUserData\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\
) ELSE IF %VER% == 2 (
	set APP_DIR=%USERPROFILE%\Documents\CELSYS\CLIPStudioModule\PlugIn\PAINT\%APP_NAME%\
) ELSE (
	GOTO RESTART
)

echo ------------------------------------------------------------
echo �v���O�C�����v���O�C���p�t�H���_�ɃC���X�g�[�����܂��B
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
