@echo off
REM Bot Creation Launcher - Redirects to scripts folder
echo Master Control Program - Bot Creator
echo =====================================
echo.
if "%1"=="" (
    echo Usage: create_bot.bat "BotName"
    echo Example: create_bot.bat "My_Sensor_Bot"
    echo.
    pause
    exit /b 1
)

echo Launching bot creation script...
call scripts\create_bot.bat %1