@echo off
set "MSYSTEM="
set "MSYSTEM_PREFIX="
set "MSYSTEM_CARCH="
set "MSYSTEM_CHOST="
call "C:\Espressif\frameworks\esp-idf-v5.5.4\export.bat"
where idf.py
if errorlevel 1 (
  echo === IDF SETUP FAILED ===
  exit /b 1
)
cd /d "%~dp0address_programmer"
echo === BUILD START ===
call idf.py build
set BUILD_RC=%errorlevel%
echo === BUILD RC=%BUILD_RC% ===
if not "%BUILD_RC%"=="0" exit /b %BUILD_RC%
echo === FLASH START ===
call idf.py -p COM14 flash
set FLASH_RC=%errorlevel%
echo === FLASH RC=%FLASH_RC% ===
exit /b %FLASH_RC%
