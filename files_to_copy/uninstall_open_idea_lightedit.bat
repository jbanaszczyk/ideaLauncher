@echo off
setlocal EnableExtensions

:: -----------------------------
:: Self-elevate to Administrator
:: -----------------------------
>nul 2>&1 net session
if %errorlevel% NEQ 0 (
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -ArgumentList '%*' -Verb RunAs"
  exit /b
)

echo.
echo === Uninstall "Open with IntelliJ LightEditor" (FILES ONLY) context menu ===

reg delete "HKCR\*\shell\open_with_idea_light" /f >nul 2>&1

echo Done.
exit /b 0
