@echo off
rem Build QuestEditor.exe with the VS C++ toolset (no solution needed), then
rem run the pack round-trip self-test against ReleaseData's tutorialetc.rpk.
rem Paths derive from this script's location. Visual Studio is located by
rem scanning the standard install folders for vcvars32.bat.
setlocal

set "HERE=%~dp0"
if "%HERE:~-1%"=="\" set "HERE=%HERE:~0,-1%"

set "VCVARS="
for %%Y in (2022 2019 2017) do (
    for %%E in (Enterprise Professional Community BuildTools Preview) do (
        if not defined VCVARS if exist "%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat"
        if not defined VCVARS if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvars32.bat"
    )
)
if not defined VCVARS (
    for /d %%V in ("%ProgramFiles%\Microsoft Visual Studio\*") do (
        for %%E in (Enterprise Professional Community BuildTools Preview) do (
            if not defined VCVARS if exist "%%~V\%%E\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=%%~V\%%E\VC\Auxiliary\Build\vcvars32.bat"
        )
    )
)
if not defined VCVARS (
    echo ERROR: no Visual Studio C++ toolset found ^(vcvars32.bat^).
    exit /b 1
)
echo using %VCVARS%
call "%VCVARS%" >nul

if not exist "%HERE%\obj" mkdir "%HERE%\obj"
set "OPTS=/nologo /std:c++17 /O2 /MT /EHsc /W3 /DUNICODE /D_UNICODE /DWIN32 /D_WINDOWS /D_CRT_SECURE_NO_WARNINGS"

cl %OPTS% /Fo"%HERE%\obj\\" /Fe"%HERE%\QuestEditor.exe" "%HERE%\main.cpp" "%HERE%\QuestXml.cpp" "%HERE%\QuestSchema.cpp" "%HERE%\RarPack.cpp" "%HERE%\urarlib.cpp" /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED comctl32.lib comdlg32.lib shell32.lib shlwapi.lib user32.lib gdi32.lib
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

echo.
echo built: %HERE%\QuestEditor.exe

set "PACK=%HERE%\..\..\ReleaseData\Data\ui\txt\tutorialetc.rpk"
if not exist "%PACK%" goto :done
echo.
echo == self-test: unpack, repack and read back %PACK%
"%HERE%\QuestEditor.exe" --roundtrip "%PACK%" --out "%HERE%\obj\roundtrip.txt"
set "RC=%errorlevel%"
type "%HERE%\obj\roundtrip.txt"
if not "%RC%"=="0" (
    echo SELF-TEST FAILED
    exit /b 1
)
:done
endlocal
