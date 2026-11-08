@echo off
setlocal EnableDelayedExpansion

echo ===============================================================================
echo            Atelier's Vault - Windows Packaging and Deployment Script
echo ===============================================================================
echo.

:: Determine Project Root directory (parent of scripts/)
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"

:: Target output paths
set "DIST_DIR=%PROJECT_ROOT%\out\dist"
set "STAGE_DIR=%DIST_DIR%\AtelierVault"
set "ZIP_FILE=%DIST_DIR%\AtelierVault-v1.0.0-Windows-x64-Portable.zip"

:: -----------------------------------------------------------------------------
:: Step 1: Detect Visual Studio and set VCINSTALLDIR (for windeployqt runtime)
:: -----------------------------------------------------------------------------
if not defined VCINSTALLDIR (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\VC" (
        set "VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC" (
        set "VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC" (
        set "VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC" (
        set "VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\"
    )
)

:: -----------------------------------------------------------------------------
:: Step 2: Resolve Qt bin directory and windeployqt.exe
:: -----------------------------------------------------------------------------
set "QT_BIN_DIR=%~1"
if "%QT_BIN_DIR%"=="" set "QT_BIN_DIR=C:\Qt\6.11.1\msvc2022_64\bin"

set "WINDEPLOYQT_EXE="
if exist "%QT_BIN_DIR%\windeployqt.exe" (
    set "WINDEPLOYQT_EXE=%QT_BIN_DIR%\windeployqt.exe"
) else (
    where windeployqt.exe >nul 2>&1
    if not errorlevel 1 (
        for /f "delims=" %%I in ('where windeployqt.exe') do (
            if not defined WINDEPLOYQT_EXE set "WINDEPLOYQT_EXE=%%I"
        )
    )
)

if not defined WINDEPLOYQT_EXE (
    echo [ERROR] windeployqt.exe could not be found!
    echo Checked: "%QT_BIN_DIR%\windeployqt.exe" and system PATH.
    echo Please supply the Qt bin directory as the first parameter:
    echo   package_windows.bat "C:\Qt\<version>\<compiler>\bin"
    exit /b 1
)
echo [OK] Found windeployqt: "%WINDEPLOYQT_EXE%"

:: -----------------------------------------------------------------------------
:: Step 3: Resolve Libsodium DLL
:: -----------------------------------------------------------------------------
set "SODIUM_PARAM=%~2"
set "LIBSODIUM_DLL="

if not "%SODIUM_PARAM%"=="" (
    if exist "%SODIUM_PARAM%" (
        rem Check if argument points directly to a file
        if not exist "%SODIUM_PARAM%\" (
            set "LIBSODIUM_DLL=%SODIUM_PARAM%"
        ) else (
            if exist "%SODIUM_PARAM%\libsodium.dll" set "LIBSODIUM_DLL=%SODIUM_PARAM%\libsodium.dll"
            if exist "%SODIUM_PARAM%\x64\Release\v143\dynamic\libsodium.dll" set "LIBSODIUM_DLL=%SODIUM_PARAM%\x64\Release\v143\dynamic\libsodium.dll"
            if exist "%SODIUM_PARAM%\bin\libsodium.dll" set "LIBSODIUM_DLL=%SODIUM_PARAM%\bin\libsodium.dll"
        )
    )
)

if not defined LIBSODIUM_DLL (
    rem Check common system and project paths
    if exist "C:\libsodium\x64\Release\v143\dynamic\libsodium.dll" (
        set "LIBSODIUM_DLL=C:\libsodium\x64\Release\v143\dynamic\libsodium.dll"
    ) else if exist "C:\libsodium\x64\Release\v142\dynamic\libsodium.dll" (
        set "LIBSODIUM_DLL=C:\libsodium\x64\Release\v142\dynamic\libsodium.dll"
    ) else if exist "C:\libsodium\bin\libsodium.dll" (
        set "LIBSODIUM_DLL=C:\libsodium\bin\libsodium.dll"
    ) else if exist "%PROJECT_ROOT%\out\build\x64-Release\libsodium.dll" (
        set "LIBSODIUM_DLL=%PROJECT_ROOT%\out\build\x64-Release\libsodium.dll"
    ) else if exist "%PROJECT_ROOT%\out\build\x64-Debug\libsodium.dll" (
        set "LIBSODIUM_DLL=%PROJECT_ROOT%\out\build\x64-Debug\libsodium.dll"
    )
)

if not defined LIBSODIUM_DLL (
    echo [ERROR] libsodium.dll was not found!
    echo Please supply the Libsodium folder or DLL path as the second parameter:
    echo   package_windows.bat [QtBin] "C:\libsodium\x64\Release\v143\dynamic"
    exit /b 1
)
echo [OK] Found libsodium.dll: "%LIBSODIUM_DLL%"

:: -----------------------------------------------------------------------------
:: Step 4: Resolve AtelierVault.exe Binary
:: -----------------------------------------------------------------------------
set "CUSTOM_BUILD_DIR=%~3"
set "APP_EXE="

if not "%CUSTOM_BUILD_DIR%"=="" (
    if exist "%CUSTOM_BUILD_DIR%\AtelierVault.exe" (
        set "APP_EXE=%CUSTOM_BUILD_DIR%\AtelierVault.exe"
    ) else if exist "%CUSTOM_BUILD_DIR%" (
        if not exist "%CUSTOM_BUILD_DIR%\" set "APP_EXE=%CUSTOM_BUILD_DIR%"
    )
)

if not defined APP_EXE (
    if exist "%PROJECT_ROOT%\out\build\x64-Release\AtelierVault.exe" (
        set "APP_EXE=%PROJECT_ROOT%\out\build\x64-Release\AtelierVault.exe"
    ) else if exist "%PROJECT_ROOT%\out\build\vs\Release\AtelierVault.exe" (
        set "APP_EXE=%PROJECT_ROOT%\out\build\vs\Release\AtelierVault.exe"
    ) else if exist "%PROJECT_ROOT%\build-win64\Release\AtelierVault.exe" (
        set "APP_EXE=%PROJECT_ROOT%\build-win64\Release\AtelierVault.exe"
    ) else if exist "%PROJECT_ROOT%\build\Release\AtelierVault.exe" (
        set "APP_EXE=%PROJECT_ROOT%\build\Release\AtelierVault.exe"
    ) else if exist "%PROJECT_ROOT%\build-win64\AtelierVault.exe" (
        set "APP_EXE=%PROJECT_ROOT%\build-win64\AtelierVault.exe"
    ) else if exist "%PROJECT_ROOT%\out\build\x64-Debug\AtelierVault.exe" (
        echo [WARNING] Release executable not found, falling back to x64-Debug binary!
        set "APP_EXE=%PROJECT_ROOT%\out\build\x64-Debug\AtelierVault.exe"
    )
)

if not defined APP_EXE (
    echo [ERROR] AtelierVault.exe could not be found!
    echo Please build the project in Release mode first:
    echo   cmake --build out/build/x64-Release --config Release
    exit /b 1
)
echo [OK] Found Application: "%APP_EXE%"

:: -----------------------------------------------------------------------------
:: Step 5: Prepare Isolated Staging Directory
:: -----------------------------------------------------------------------------
echo.
echo [*] Cleaning and preparing staging directory: "%STAGE_DIR%"...
if exist "%STAGE_DIR%" (
    rmdir /s /q "%STAGE_DIR%"
    if errorlevel 1 (
        echo [ERROR] Failed to clean existing staging directory. Is a file locked?
        exit /b 1
    )
)
mkdir "%STAGE_DIR%"
if not exist "%DIST_DIR%" mkdir "%DIST_DIR%"

:: -----------------------------------------------------------------------------
:: Step 6: Copy Core Binaries and Resources
:: -----------------------------------------------------------------------------
echo [*] Copying core application and cryptographic binaries...
copy /y "%APP_EXE%" "%STAGE_DIR%\AtelierVault.exe" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy AtelierVault.exe to staging.
    exit /b 1
)

copy /y "%LIBSODIUM_DLL%" "%STAGE_DIR%\libsodium.dll" >nul
if errorlevel 1 (
    echo [ERROR] Failed to copy libsodium.dll to staging.
    exit /b 1
)

if exist "%PROJECT_ROOT%\icon.ico" (
    copy /y "%PROJECT_ROOT%\icon.ico" "%STAGE_DIR%\icon.ico" >nul
)
if exist "%PROJECT_ROOT%\icon.png" (
    copy /y "%PROJECT_ROOT%\icon.png" "%STAGE_DIR%\icon.png" >nul
)

:: -----------------------------------------------------------------------------
:: Step 7: Run windeployqt
:: -----------------------------------------------------------------------------
echo [*] Running windeployqt to bundle Qt dependencies and MSVC runtime...
"%WINDEPLOYQT_EXE%" --release --compiler-runtime --no-translations "%STAGE_DIR%\AtelierVault.exe"
if errorlevel 1 (
    echo [ERROR] windeployqt failed with error code %errorlevel%.
    exit /b %errorlevel%
)
echo [OK] Qt dependencies successfully deployed.

:: Bundle MSVC runtime DLLs directly into staging if available
for /d %%D in ("%VCINSTALLDIR%Redist\MSVC\*") do (
    if exist "%%D\x64\Microsoft.VC*.CRT" (
        copy /y "%%D\x64\Microsoft.VC*.CRT\*.dll" "%STAGE_DIR%\" >nul 2>&1
    )
)

:: -----------------------------------------------------------------------------
:: Step 8: Create Portable ZIP Archive
:: -----------------------------------------------------------------------------
echo.
echo [*] Creating portable ZIP archive: "%ZIP_FILE%"...
if exist "%ZIP_FILE%" del /f /q "%ZIP_FILE%"

powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command ^
    "try { Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::CreateFromDirectory('%STAGE_DIR%', '%ZIP_FILE%'); exit 0 } catch { Write-Host 'Zip Error:' $_.Exception.Message; exit 1 }"

if errorlevel 1 (
    echo [ERROR] Failed to create portable zip archive.
    exit /b 1
)

echo.
echo ===============================================================================
echo [SUCCESS] Windows distribution staged and packaged successfully!
echo   Staging Directory : %STAGE_DIR%
echo   Portable ZIP File : %ZIP_FILE%
echo ===============================================================================
exit /b 0
