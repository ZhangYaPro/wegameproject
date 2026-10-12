@echo off
setlocal
set "SKI_ENGINE=D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe"
if not exist "%SKI_ENGINE%" (
  echo Unreal Engine 5.5 was not found at D:\UE_5.5.
  pause
  exit /b 1
)
start "Ski25D" "%SKI_ENGINE%" "%~dp0Ski25D.uproject" /Game/Ski25D/Maps/SkiRimeMap -game -windowed -ResX=1280 -ResY=720 -ForceRes
