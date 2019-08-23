@ECHO OFF

SET forcevdk=
SET forcevdkText=

:MENU
ECHO Select the type of project you would like to create:
IF "%forcevdk%" =="" ECHO 0. Euclideon VDK Developers (Additional Menu)
ECHO 1. Visual Studio 2019 Solution%forcevdkText% (DirectX 11)
ECHO 2. Visual Studio 2019 Solution%forcevdkText% (OpenGL)
ECHO 3. Visual Studio 2015 Solution%forcevdkText% (DirectX 11)
ECHO 4. Visual Studio 2015 Solution%forcevdkText% (OpenGL)

IF "%forcevdk%" =="" CHOICE /N /C:12340 /M "[0-4]:"
IF "%forcevdk%" NEQ "" CHOICE /N /C:1234 /M "[1-4]:"

IF ERRORLEVEL ==5 GOTO ZERO
IF ERRORLEVEL ==4 GOTO FOUR
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
 ECHO Creating VS2015%forcevdkText% DirectX Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 %forcevdk% --gfxapi=d3d11
 GOTO END

:FOUR
 ECHO Creating VS2015%forcevdkText% OpenGL Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 %forcevdk% --gfxapi=opengl
 GOTO END

:ZERO
 SET forcevdk=--force-vaultsdk
 SET forcevdkText= with Vault source code
 GOTO MENU

:END
