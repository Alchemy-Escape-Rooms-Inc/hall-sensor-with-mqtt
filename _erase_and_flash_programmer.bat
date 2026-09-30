@echo off
setlocal
set "MSYSTEM="
set "MSYSTEM_PREFIX="
set "MSYSTEM_CARCH="
set "MSYSTEM_CHOST="

set IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.5.4
set IDF_TOOLS_PATH=C:\Espressif
set IDF_PYTHON_ENV_PATH=C:\Espressif\python_env\idf5.5_py3.13_env
set "PATH=%IDF_PYTHON_ENV_PATH%\Scripts"
set "PATH=%PATH%;%IDF_PATH%\tools"
set "PATH=%PATH%;%IDF_PATH%\components\espcoredump"
set "PATH=%PATH%;%IDF_PATH%\components\partition_table"
set "PATH=%PATH%;%IDF_PATH%\components\app_update"
set "PATH=%PATH%;%IDF_PATH%\components\esptool_py\esptool"
set "PATH=%PATH%;C:\Espressif\tools\cmake\3.30.2\bin"
set "PATH=%PATH%;C:\Espressif\tools\ninja\1.12.1"
set "PATH=%PATH%;C:\Espressif\tools\ccache\4.12.1"
set "PATH=%PATH%;C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin"
set "PATH=%PATH%;C:\Espressif\tools\esp32ulp-elf\2.38_20240113\esp32ulp-elf\bin"
set "PATH=%PATH%;C:\Windows\System32;C:\Windows;C:\Windows\System32\WindowsPowerShell\v1.0"

set PORT=%1
if "%PORT%"=="" set PORT=COM4

cd /d "%~dp0address_programmer"

echo === ERASE FLASH on %PORT% ===
"%IDF_PYTHON_ENV_PATH%\Scripts\python.exe" "%IDF_PATH%\tools\idf.py" -p %PORT% erase-flash
if errorlevel 1 (
  echo === ERASE FAILED ===
  exit /b 1
)

echo === BUILD ===
"%IDF_PYTHON_ENV_PATH%\Scripts\python.exe" "%IDF_PATH%\tools\idf.py" build
if errorlevel 1 (
  echo === BUILD FAILED ===
  exit /b 1
)

echo === FLASH on %PORT% ===
"%IDF_PYTHON_ENV_PATH%\Scripts\python.exe" "%IDF_PATH%\tools\idf.py" -p %PORT% flash
if errorlevel 1 (
  echo === FLASH FAILED ===
  exit /b 1
)
echo === DONE ===
