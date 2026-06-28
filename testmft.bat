@echo off
cd /d "%~dp0"

:: Компиляция с выводом в текущую консоль (VSCode видит)
call ./buildmft.bat
if %errorlevel% neq 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

:: Проверка прав администратора
net session >nul 2>&1
if %errorlevel% neq 0 (
    goto UACPrompt
) else (
    goto gotAdmin
)

:UACPrompt
    echo Requesting administrator privileges...
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
    "%temp%\getadmin.vbs"
    exit /b

:gotAdmin
    if exist "%temp%\getadmin.vbs" del "%temp%\getadmin.vbs"
    cd /d "%~dp0"
    cls
    "mft.exe"
    pause