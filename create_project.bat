@ECHO OFF

SET forceudSDK=
SET forceudSDKText=

:MENU
ECHO Select the type of project you would like to create:
IF "%forceudSDK%" =="" ECHO 0. Euclideon VDK Developers (Additional Menu)
ECHO 1. Visual Studio 2019 Solution%forceudSDKText% (DirectX 11)
ECHO 2. Visual Studio 2019 Solution%forceudSDKText% (OpenGL)
ECHO 3. Visual Studio 2019 Solution%forceudSDKText% (OpenGLES+Android)

IF "%forceudSDK%" =="" CHOICE /N /C:1230 /M "[0-3]:"
IF "%forceudSDK%" NEQ "" CHOICE /N /C:123 /M "[1-3]:"

IF ERRORLEVEL ==4 GOTO ZERO
IF ERRORLEVEL ==3 GOTO THREE
IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE

GOTO END

:ONE
 ECHO Creating VS2019%forceudSDKText% DirectX Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forceudSDK% --gfxapi=d3d11
 GOTO END

:TWO
 ECHO Creating VS2019%forceudSDKText% OpenGL Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forceudSDK% --gfxapi=opengl
 GOTO END

:THREE
 ECHO Creating VS2019%forceudSDKText% OpenGL+Android Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2019 %forceudSDK% --gfxapi=opengl --os=android
 GOTO END

:ZERO
 SET forceudSDK=--force-udsdk
 SET forceudSDKText= with udSDK source code
 GOTO MENU

:END
