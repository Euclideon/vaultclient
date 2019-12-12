@ECHO OFF

SET forcevdk=
SET forcevdkText=

:MENU
ECHO Select the type of project you would like to create:
IF "%forcevdk%" =="" ECHO 0. Euclideon VDK Developers (Additional Menu)
ECHO 1. Visual Studio 2019 Solution%forcevdkText% (DirectX 11)
ECHO 2. Visual Studio 2019 Solution%forcevdkText% (OpenGL)
ECHO 3. Visual Studio 2019 Solution%forcevdkText% (OpenGLES+Android)

IF "%forcevdk%" =="" CHOICE /N /C:1230 /M "[0-3]:"
IF "%forcevdk%" NEQ "" CHOICE /N /C:123 /M "[1-3]:"

IF ERRORLEVEL ==4 GOTO ZERO
IF ERRORLEVEL ==3 GOTO THREE
IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE

GOTO END

:ONE
 ECHO Creating VS2019%forcevdkText% DirectX Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forcevdk% --gfxapi=d3d11
 GOTO END

:TWO
 ECHO Creating VS2019%forcevdkText% OpenGL Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forcevdk% --gfxapi=opengl
 GOTO END

:THREE
 ECHO Creating VS2019%forcevdkText% OpenGL+Android Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forcevdk% --gfxapi=opengl --os=android
 GOTO END

:ZERO
 SET forcevdk=--force-vaultsdk
 SET forcevdkText= with Vault source code
 GOTO MENU

:END
