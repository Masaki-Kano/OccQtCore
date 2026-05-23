@echo off
setlocal

set OCC_BIN=%~1
set OCC_3RDPARTY=%~2
set TARGET_DIR=%~3

echo [INFO] OCC_BIN      = %OCC_BIN%
echo [INFO] 3RDPARTY_DIR = %OCC_3RDPARTY%
echo [INFO] TARGET_DIR   = %TARGET_DIR%

if not exist "%OCC_BIN%" (
    echo [ERROR] OCC_BIN not found: %OCC_BIN%
    exit /b 1
)

echo [INFO] Copying OCCT DLLs...
xcopy /Y /D "%OCC_BIN%\*.dll" "%TARGET_DIR%\"

if exist "%OCC_3RDPARTY%" (
    echo [INFO] Copying 3rdparty DLLs...
    for /R "%OCC_3RDPARTY%" %%f in (*.dll) do (
        echo [COPY] %%f
        copy /Y "%%f" "%TARGET_DIR%\" >nul
    )
) else (
    echo [WARN] 3rdparty folder not found: %OCC_3RDPARTY%
)

echo [INFO] OCCT deploy done.
endlocal
