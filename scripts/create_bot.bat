@echo off
setlocal

if "%1"=="" (
    echo Usage: create_bot.bat "Bot_Name"
    echo Example: create_bot.bat "Weather_Station_01"
    exit /b 1
)

set BOT_NAME=%~1
set SCRIPT_DIR=%~dp0
set PROJECT_ROOT=%SCRIPT_DIR%..
set BOT_DIR=%PROJECT_ROOT%\esp32_examples\bots\%BOT_NAME%
set TEMPLATE_DIR=%PROJECT_ROOT%\esp32_examples\generic_bot_template

echo Creating new ESP32 bot: %BOT_NAME%

if not exist "%TEMPLATE_DIR%" (
    echo Error: Template not found at %TEMPLATE_DIR%
    exit /b 1
)

if exist "%BOT_DIR%" (
    set /p OVERWRITE="Bot already exists. Overwrite? (y/N): "
    if /i not "!OVERWRITE!"=="y" (
        echo Bot creation cancelled
        exit /b 1
    )
    rmdir /s /q "%BOT_DIR%"
)

echo Copying template...
xcopy "%TEMPLATE_DIR%" "%BOT_DIR%" /e /i /q

echo Updating bot configuration...
powershell -Command "(Get-Content '%BOT_DIR%\src\config.h') -replace 'const char\* BOT_CUSTOM_NAME = \"Generic_Bot\";', 'const char* BOT_CUSTOM_NAME = \"%BOT_NAME%\";' | Set-Content '%BOT_DIR%\src\config.h'"

echo.
echo Bot created successfully!
echo Location: %BOT_DIR%
echo.
echo Next Steps:
echo 1. cd "%BOT_DIR%"
echo 2. Edit src\config.h with your WiFi credentials
echo 3. Customize sensor code in src\main.cpp
echo 4. Build: pio run
echo 5. Upload: pio run --target upload --upload-port COM3
echo.

set /p OPEN_FOLDER="Open bot folder? (y/N): "
if /i "%OPEN_FOLDER%"=="y" (
    start "" "%BOT_DIR%"
)

echo Bot '%BOT_NAME%' is ready for development!