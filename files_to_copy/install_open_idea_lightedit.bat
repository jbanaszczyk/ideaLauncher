@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: -----------------------------
:: Self-elevate to Administrator
:: -----------------------------
>nul 2>&1 net session
if %errorlevel% NEQ 0 (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -ArgumentList '%*' -Verb RunAs"
  exit /b
)

echo.
echo === Install "Open with IntelliJ LightEditor" context menu ===

:: -----------------------------
:: Get LAUNCHER_EXE from arg or prompt
:: -----------------------------
set "LAUNCHER_EXE=C:\Program Files\JetBrains\LightEdit\ideaLauncher.exe"


if not exist "%LAUNCHER_EXE%" (
  echo [ERROR] Launcher not found at: "%LAUNCHER_EXE%"
  echo        Expected default path: C:\Program Files\JetBrains\LightEdit\ideaLauncher.exe
  exit /b 1
)

echo Using: "%LAUNCHER_EXE%"
echo.

set "ICON=%LAUNCHER_EXE%"

:: -----------------------------
:: Add for ALL FILES (*)
:: -----------------------------
reg add "HKCR\*\shell\open_with_idea_light" /ve /d "Open with IntelliJ LightEditor" /f >nul
reg add "HKCR\*\shell\open_with_idea_light" /v "Icon" /t REG_SZ /d "\"%ICON%\"" /f >nul
reg add "HKCR\*\shell\open_with_idea_light\command" /ve /t REG_SZ /d "\"%LAUNCHER_EXE%\" \"%%1\"" /f >nul

echo.
echo Done. Right-click a file - Show more options - Open with IntelliJ LightEditor
exit /b 0
