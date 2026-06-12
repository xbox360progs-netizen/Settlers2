@echo off

setlocal

if not exist GestureBuilderDataFileUtilities.exe (echo Cannot find GestureBuilderDataFileUtilities.exe
exit /b -1)

if not exist PCAProgressTrainer.exe (echo Cannot find PCAProgressTrainer.exe
exit /b -1)

:GENTEMP
set TEMPFILE="%TEMP%\PCAProgress~tmp~%~n2.txt"
set TEMPFOLDER="%TEMP%\PCAProgress%~n2"

if exist %TEMPFILE% goto GENTEMP
if exist %TEMPFOLDER% goto GENTEMP

GestureBuilderDataFileUtilities.exe GET_PROJECT_DATA %2 %TEMPFILE%
set OUTERROR=%ERRORLEVEL%

if %ERRORLEVEL% EQU 0 goto TRAIN
if %ERRORLEVEL% EQU -2 goto TRAIN
goto END

:TRAIN

md %TEMPFOLDER%
PCAProgressTrainer.exe %TEMPFILE% %TEMPFOLDER%
set OUTERROR=%ERRORLEVEL%

if %ERRORLEVEL% EQU 0 goto PACK
if %ERRORLEVEL% EQU -2 goto PACK
goto END

:PACK

GestureBuilderDataFileUtilities.exe PACK_DIRECTORY %TEMPFOLDER% %3
set OUTERROR=%ERRORLEVEL%

:END

if exist %TEMPFOLDER% (rd /s /q %TEMPFOLDER%)
if exist %TEMPFILE% (del /q %TEMPFILE%)

exit /b %OUTERROR%
