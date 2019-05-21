@ECHO OFF

ECHO Select the type of project you would like to create:
ECHO 1. Visual Studio 2015 Solution
ECHO 2. Visual Studio 2015 Solution With vaultSDK Repository
ECHO 3. Visual Studio 2015 Solution (DirectX 11)
ECHO 4. Visual Studio 2015 Solution With vaultSDK Repository (DirectX 11)
ECHO 5. Visual Studio 2015 Solution With vaultSDK Repository (Emscripten)

CHOICE /N /C:12345 /M "[1-5]:"

IF ERRORLEVEL ==5 GOTO FIVE
IF ERRORLEVEL ==4 GOTO FOUR
IF ERRORLEVEL ==3 GOTO THREE
IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE
GOTO END

:ONE
 ECHO Creating VS2015 Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015
 GOTO END

:TWO
 ECHO Creating VS2015 Project (With vaultSDK Repository)...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 --force-vaultsdk
 GOTO END

:THREE
 ECHO Creating VS2015 Project...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 --gfxapi=d3d11
 GOTO END

:FOUR
 ECHO Creating VS2015 Project (With vaultSDK Repository)...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 --force-vaultsdk --gfxapi=d3d11
 GOTO END

:FIVE
 ECHO Creating VS2015 Project (With vaultSDK Repository) for Emscripten...
 3rdParty\udcore\bin\premake-bin\premake5.exe vs2015 --force-vaultsdk --os=emscripten
 GOTO END

:END
