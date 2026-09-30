@echo off
set "MSYSTEM="
set "MSYSTEM_PREFIX="
call "C:\Espressif\frameworks\esp-idf-v5.5.4\export.bat" >nul 2>&1
python "%~dp0_serial_capture.py" %1 %2
