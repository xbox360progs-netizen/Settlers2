@echo off
setlocal

if not exist GestureBuilderDataFileUtilities.exe (echo Cannot find GestureBuilderDataFileUtilities.exe
exit /b -1)

if not exist RFRProgressTrainer.exe (echo Cannot find RFRProgressTrainer.exe
exit /b -1)

:GENTEMP
set TEMPFILE="%TEMP%\RFRProgress~tmp~%~n2.txt"

if exist %TEMPFILE% goto GENTEMP
GestureBuilderDataFileUtilities.exe GET_PROJECT_DATA "%2" %TEMPFILE%
set OUTERROR=%ERRORLEVEL%

if %ERRORLEVEL% EQU 0 goto TRAIN
if %ERRORLEVEL% EQU -2 goto TRAIN
goto END

:TRAIN
RFRProgressTrainer.exe %TEMPFILE% "%3" "%~dpnx2_meta"
set OUTERROR=%ERRORLEVEL%

:END
if exist %TEMPFILE% (del /q %TEMPFILE%)
exit /b %OUTERROR%