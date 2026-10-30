@echo off
cd /d "%~dp0"
where node >nul 2>nul
if %errorlevel%==0 (
  node ..\..\engine\script\js\build.js %*
) else (
  ..\..\tools\build\Node\node.exe ..\..\engine\script\js\build.js %*
)
