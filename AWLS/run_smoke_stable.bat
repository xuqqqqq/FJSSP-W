@echo off
setlocal EnableExtensions EnableDelayedExpansion

REM Run from AWLS directory
set "ROOT=%~dp0"
cd /d "%ROOT%"

set "EXE=x64\Release\AWLS.exe"
if not exist "%EXE%" (
  echo ERROR: %EXE% not found. Build Release x64 first.
  exit /b 1
)

set "INST_DIR=..\instances\Example_Instances_FJSSP-WF"
set "OUT_CSV=smoke_results_stable.csv"
set "TIME_LIMIT=10"
set "SEED=1234"

> "%OUT_CSV%" echo instance,seed,time_limit_sec,makespan,status

call :run_one Fattahi1.fjs
call :run_one Fattahi2.fjs
call :run_one Fattahi3.fjs
call :run_one Fattahi5.fjs
call :run_one Kacem1.fjs
call :run_one Kacem2.fjs
call :run_one Kacem3.fjs

echo.
echo Done. Results: %OUT_CSV%
exit /b 0

:run_one
set "INST=%~1"
set "TMP=smoke_tmp_%~n1.txt"

echo Running %INST% ...
cmd /c "type "%INST_DIR%\%INST%" | %EXE% %TIME_LIMIT% %SEED%" > "%TMP%" 2>&1
set "RC=%ERRORLEVEL%"

set "MK="
for /f "tokens=3" %%A in ('findstr /c:"Final solution:" "%TMP%"') do set "MK=%%A"

if "%RC%"=="0" (
  if defined MK (
    >> "%OUT_CSV%" echo %INST%,%SEED%,%TIME_LIMIT%,%MK%,OK
  ) else (
    >> "%OUT_CSV%" echo %INST%,%SEED%,%TIME_LIMIT%,,NO_FINAL_LINE
  )
) else (
  >> "%OUT_CSV%" echo %INST%,%SEED%,%TIME_LIMIT%,,CRASH_OR_ERROR_%RC%
)

del "%TMP%" >nul 2>nul
exit /b 0
