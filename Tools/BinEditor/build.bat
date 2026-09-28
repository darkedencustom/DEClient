@echo off
rem Build BinEditor.exe from bin_editor.py with PyInstaller (pip install pyinstaller).
rem The exe is a single windowed file; names.json next to it supplies the item/option/monster names.
setlocal
set "HERE=%~dp0"
if "%HERE:~-1%"=="\" set "HERE=%HERE:~0,-1%"
cd /d "%HERE%"

python -m PyInstaller --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: PyInstaller is not installed.  Run:  pip install pyinstaller
    exit /b 1
)

echo == self-test (parses the data folder, exercises the window without showing it)
set "BIN_EDITOR_SELFTEST=1"
python bin_editor.py
if errorlevel 1 (
    echo SELF-TEST FAILED
    exit /b 1
)
set "BIN_EDITOR_SELFTEST="

echo == building
python -m PyInstaller --noconfirm --clean --onefile --windowed --name BinEditor ^
    --distpath "%HERE%" --workpath "%HERE%\build" --specpath "%HERE%\build" bin_editor.py
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
rmdir /s /q "%HERE%\build" >nul 2>&1

echo.
echo built: %HERE%\BinEditor.exe
endlocal
