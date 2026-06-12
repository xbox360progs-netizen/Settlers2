@echo off

if '%1' == '' (echo Usage: %~nx0 gesture.gbd
exit /b)

setlocal

set PATH=%PATH%;"%XEDK%\bin\win32"
set OBJDIR=%~dp0VisualGestureBuilderPreview
set TARGETNAME=VisualGestureBuilderPreview
set DESTDIR=xe:\%TARGETNAME%

if not exist "%OBJDIR%\%TARGETNAME%.xex" goto MissingXex

if /i "%~x1" equ ".gba" goto Process
if /i "%~x1" equ ".gbd" goto Process

echo %1 is not a gesture database file
exit /b -1

:Process
if not exist %1 goto MissingGbd
if not exist "%~dpn1.h" goto MissingGbd

echo ------------------------
echo Creating directory on xbox: %DESTDIR%
echo ------------------------
xbmkdir %DESTDIR%

echo ------------------------
echo Copying files to xbox: %DESTDIR%
echo ------------------------

xbcp /Y /T /D "%XEDK%\redist\xbox\NaturalInput\Databases\*.*" %DESTDIR%\
xbcp /Y /T /S /D "%OBJDIR%\*.*" %DESTDIR%\
xbcp /Y /T %1 %DESTDIR%\Media\Gestures\
xbcp /Y /T "%~dpn1.h" %DESTDIR%\Media\Gestures\

echo ------------------------
echo Launching on devkit...
echo ------------------------

xbreboot %DESTDIR%\%TARGETNAME%.xex "%~nx1"

exit /b

:MissingXex
echo Cannot find necessary files from XDK
exit /b -1

:MissingGbd
echo Cannot find the database or header file for %1
exit /b -1
