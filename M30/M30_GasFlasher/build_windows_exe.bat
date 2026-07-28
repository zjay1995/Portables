@echo off
REM ---------------------------------------------------------------------------
REM Builds M30Flasher.exe (single file, no console window) with PyInstaller.
REM Run this on the Windows PC that will hand out the .exe.
REM Requires: Python 3.10+ for Windows (the python.org installer, which ships
REM tkinter).  Everything else is installed here.
REM ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0"

where python >nul 2>&1
if errorlevel 1 (
  echo Python was not found on PATH. Install Python 3.10+ from python.org first.
  exit /b 1
)

echo Installing build dependencies...
python -m pip install --upgrade pip                 || exit /b 1
python -m pip install pyinstaller pyserial          || exit /b 1

echo Building M30Flasher.exe ...
python -m PyInstaller --noconfirm --clean M30Flasher.spec || exit /b 1

echo.
echo Done. The program is at: dist\M30Flasher.exe
echo Copy arduino-cli.exe into a "tools" folder next to it to make it portable.
endlocal
