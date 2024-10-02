@echo off
setlocal EnableDelayedExpansion

set "lastLine="
set "cline=dummyData"

set "p_f=false"
if "%~1"=="-l" set "p_f=true"
if "%~1"=="--list" set "p_f=true"

if "%p_f%"=="true" (
    "%~dp0xcd-a.exe" %*
    exit /b
)

for /f "delims=" %%i in ('"%~dp0xcd-a.exe" %*') do (
    set "cline=%%i"
    if "!cline:~0,7!"=="~$$;cd;" (
        set "lastLine=!cline:~7!" REM if it has the cd prefix then set it as the last line
    ) else (
        echo !cline!
    )
)

REM echo [xcd][BATCH-DEBUG] lastLine=!lastLine!
if defined lastLine (
    if "!lastLine!" NEQ "" (
        REM echo [xcd][BATCH-DEBUG] cd "!lastLine!" ^2^>nul
        endlocal
        cd "%lastLine%" 2>nul
        exit /b
    )
)
