@echo off
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   Hall Sensor Address Programmer
echo ========================================
echo.

:: =============================================
:: CHECK FOR COMMAND LINE ARGUMENTS
:: =============================================
:: Usage: program_sensor.bat [slot] [COM_PORT]
:: Example: program_sensor.bat 3 COM4

set "ARG_SLOT=%~1"
set "ARG_PORT=%~2"

:: =============================================
:: AUTO-DETECT AND SETUP ESP-IDF ENVIRONMENT
:: =============================================

:: Check if idf.py is already available
where idf.py >nul 2>&1
if %errorlevel% equ 0 (
    echo ESP-IDF environment detected.
    goto :idf_ready
)

echo Setting up ESP-IDF environment...
echo.

:: Try common ESP-IDF installation paths
set "IDF_FOUND=0"

:: Path 0: ESP-IDF v5.5.4 (YOUR INSTALLATION)
if exist "C:\Espressif\frameworks\esp-idf-v5.5.4\export.bat" (
    echo Found ESP-IDF v5.5.4 at C:\Espressif\frameworks\esp-idf-v5.5.4
    call "C:\Espressif\frameworks\esp-idf-v5.5.4\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 1: Default Espressif installer location
if exist "C:\Espressif\frameworks\esp-idf\export.bat" (
    echo Found ESP-IDF at C:\Espressif\frameworks\esp-idf
    call "C:\Espressif\frameworks\esp-idf\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 2: Older Espressif installer
if exist "C:\Espressif\esp-idf\export.bat" (
    echo Found ESP-IDF at C:\Espressif\esp-idf
    call "C:\Espressif\esp-idf\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 3: User home esp folder
if exist "%USERPROFILE%\esp\esp-idf\export.bat" (
    echo Found ESP-IDF at %USERPROFILE%\esp\esp-idf
    call "%USERPROFILE%\esp\esp-idf\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 4: C:\esp-idf
if exist "C:\esp-idf\export.bat" (
    echo Found ESP-IDF at C:\esp-idf
    call "C:\esp-idf\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 5: C:\esp\esp-idf
if exist "C:\esp\esp-idf\export.bat" (
    echo Found ESP-IDF at C:\esp\esp-idf
    call "C:\esp\esp-idf\export.bat"
    set "IDF_FOUND=1"
    goto :check_idf
)

:: Path 6: Check IDF_PATH environment variable
if defined IDF_PATH (
    if exist "%IDF_PATH%\export.bat" (
        echo Found ESP-IDF at %IDF_PATH%
        call "%IDF_PATH%\export.bat"
        set "IDF_FOUND=1"
        goto :check_idf
    )
)

:: Not found - ask user
echo.
echo ESP-IDF not found in common locations!
echo.
echo Please enter the full path to your ESP-IDF folder
echo (the folder containing export.bat)
echo.
echo Example: C:\Espressif\frameworks\esp-idf
echo.
set /p IDF_CUSTOM_PATH="ESP-IDF path: "

if exist "%IDF_CUSTOM_PATH%\export.bat" (
    call "%IDF_CUSTOM_PATH%\export.bat"
    set "IDF_FOUND=1"
) else (
    echo.
    echo ERROR: export.bat not found at %IDF_CUSTOM_PATH%
    echo Please check the path and try again.
    pause
    exit /b 1
)

:check_idf
:: Verify idf.py is now available
where idf.py >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo ERROR: ESP-IDF setup failed. idf.py not found.
    echo Please run this from ESP-IDF Command Prompt.
    pause
    exit /b 1
)

:idf_ready
echo.
echo ========================================
echo   ESP-IDF Ready!
echo ========================================
echo.
echo This tool will program a NEW hall sensor
echo with a unique I2C address.
echo.
echo IMPORTANT: Connect ONLY the sensor you
echo want to program. Disconnect all others!
echo.
echo Available sensor slots:
echo   0 = 0x60    4 = 0x64
echo   1 = 0x61    5 = 0x65
echo   2 = 0x62    6 = 0x66
echo   3 = 0x63    7 = 0x67
echo.

:: Use command line arg or ask
if defined ARG_SLOT (
    set "SENSOR_NUM=%ARG_SLOT%"
    echo Using slot from command line: %ARG_SLOT%
    goto :validate_slot
)

:ask_slot
set /p SENSOR_NUM="Enter sensor slot (0-7): "

:validate_slot
:: Validate input
if "%SENSOR_NUM%"=="" goto ask_slot
if %SENSOR_NUM% LSS 0 (
    echo Invalid slot. Please enter 0-7.
    set "ARG_SLOT="
    goto ask_slot
)
if %SENSOR_NUM% GTR 7 (
    echo Invalid slot. Please enter 0-7.
    set "ARG_SLOT="
    goto ask_slot
)

:: Calculate target address
set /a TARGET_ADDR=96+%SENSOR_NUM%
set /a HEX_ADDR=0x60+%SENSOR_NUM%

echo.
echo Programming sensor for slot #%SENSOR_NUM% (address 0x6%SENSOR_NUM%)
echo.

:: Generate target_address.h
echo #pragma once > "%~dp0address_programmer\main\target_address.h"
echo #define TARGET_ADDRESS 0x6%SENSOR_NUM% >> "%~dp0address_programmer\main\target_address.h"

echo Generated target address header file.
echo.

:: Navigate to programmer directory
cd /d "%~dp0address_programmer"

echo Building firmware...
echo.
call idf.py build
if %errorlevel% neq 0 (
    echo.
    echo BUILD FAILED! Check errors above.
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.

:: Use command line arg or ask for COM port
if defined ARG_PORT (
    set "COM_PORT=%ARG_PORT%"
    echo Using COM port from command line: %ARG_PORT%
) else (
    set /p COM_PORT="Enter COM port (e.g., COM3): "
)

echo.
echo Flashing to %COM_PORT%...
echo.
call idf.py -p %COM_PORT% flash
if %errorlevel% neq 0 (
    echo.
    echo FLASH FAILED! Check that:
    echo   - The correct COM port is selected
    echo   - The ESP32 is connected
    echo   - No other program is using the port
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Flashing complete!
echo ========================================
echo.
echo Opening serial monitor to show results...
echo (Press Ctrl+] to exit monitor)
echo.

call idf.py -p %COM_PORT% monitor

cd /d "%~dp0"
pause
