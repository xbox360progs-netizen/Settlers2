@echo off

rem
echo.
echo Setting environment for using Microsoft Xbox 360 SDK tools.
echo.
rem
rem Enable the Speech Platform SDK tools to be run from the 360 command
set SPEECHSDK=%PROGRAMFILES%\Microsoft SDKs\Speech\v11.0\Tools
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set SPEECHSDK=%PROGRAMFILES(x86)%\Microsoft SDKs\Speech\v11.0\Tools

set PATH=%XEDK%\bin\win32;%SPEECHSDK%;%PATH%;
set INCLUDE=%XEDK%\include\win32;%XEDK%\include\xbox;%XEDK%\include\xbox\sys;%INCLUDE%
set LIB=%XEDK%\lib\win32;%XEDK%\lib\xbox;%LIB%
set _NT_SYMBOL_PATH=SRV*%XEDK%\bin\xbox\symsrv;%_NT_SYMBOL_PATH%

cd /d "%XEDK%\bin\win32"

:end
