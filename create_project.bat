@ECHO OFF

ECHO Select the type of project you would like to create:
ECHO 1. Visual Studio 2015 Solution
ECHO 2. Visual Studio 2015 Solution With vaultSDK Repository

CHOICE /N /C:12 /M "[1-2]:"

IF ERRORLEVEL ==2 GOTO TWO
IF ERRORLEVEL ==1 GOTO ONE
GOTO END

:ONE
 ECHO Creating VS2015 Project...
 bin\premake\premake5.exe vs2015
 GOTO END
 
:TWO
 ECHO Creating VS2015 Project (With vaultSDK Repository)...
 bin\premake\premake5.exe vs2015 --force-vaultsdk
 GOTO END

:END
