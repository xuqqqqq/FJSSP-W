@echo off
setlocal

if "%~1"=="" (
  echo Usage: run_awls_on_instance.bat INSTANCE_FILE [TIME_LIMIT_SEC] [SEED]
  exit /b 2
)

set "INSTANCE_FILE=%~1"
set "TIME_LIMIT=%~2"
if "%TIME_LIMIT%"=="" set "TIME_LIMIT=10"
set "SEED=%~3"
if "%SEED%"=="" set "SEED=1234"

type "%INSTANCE_FILE%" | x64\Release\AWLS.exe %TIME_LIMIT% %SEED%
exit /b %ERRORLEVEL%
