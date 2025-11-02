# ESP32 Bot Creation Script (PowerShell)
# Creates new bots from the generic template

param(
    [Parameter(Mandatory=$true)]
    [string]$BotName
)

Write-Host "🤖 Creating new ESP32 bot: $BotName" -ForegroundColor Green

# Get script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TemplateDir = Join-Path $ScriptDir "esp32_examples\generic_bot_template"
$BotDir = Join-Path $ScriptDir "esp32_examples\$($BotName.ToLower().Replace(' ', '_'))"

# Check if template exists
if (-not (Test-Path $TemplateDir)) {
    Write-Host "❌ Template not found at: $TemplateDir" -ForegroundColor Red
    exit 1
}

# Check if bot already exists
if (Test-Path $BotDir) {
    $overwrite = Read-Host "⚠️ Bot '$BotName' already exists. Overwrite? (y/N)"
    if ($overwrite.ToLower() -ne 'y') {
        Write-Host "❌ Bot creation cancelled" -ForegroundColor Red
        exit 1
    }
    Remove-Item $BotDir -Recurse -Force
}

# Copy template
try {
    Write-Host "📁 Creating bot directory: $BotDir" -ForegroundColor Yellow
    Copy-Item $TemplateDir $BotDir -Recurse
    
    # Update config.h with bot name
    $ConfigFile = Join-Path $BotDir "src\config.h"
    if (Test-Path $ConfigFile) {
        $content = Get-Content $ConfigFile -Raw -Encoding UTF8
        $content = $content -replace 'const char\* BOT_CUSTOM_NAME = "Generic_Bot";', "const char* BOT_CUSTOM_NAME = `"$BotName`";"
        Set-Content $ConfigFile $content -Encoding UTF8
        Write-Host "[OK] Updated bot name to: $BotName" -ForegroundColor Green
    }
    
    # Create simple README
    $ReadmeContent = @"
# Weather_Station_01

## Quick Start
1. Edit src/config.h with your WiFi credentials
2. Customize sensor code in src/main.cpp
3. Build and upload: pio run --target upload

## Template Features
- Pre-configured ESP-NOW communication
- MCP server integration
- OTA firmware updates
- Sensor framework ready for customization

See generic_bot_template/README.md for detailed documentation.
"@
    
    $ReadmeFile = Join-Path $BotDir "README.md"
    Set-Content $ReadmeFile $ReadmeContent -Encoding UTF8
    
    Write-Host ""
    Write-Host "🎉 Bot created successfully!" -ForegroundColor Green
    Write-Host "📁 Location: $BotDir" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "📋 Next Steps:" -ForegroundColor Yellow
    Write-Host "1. cd `"$BotDir`""
    Write-Host "2. Edit src\config.h with your WiFi credentials"
    Write-Host "3. Customize sensor code in src\main.cpp"
    Write-Host "4. Build: pio run"
    Write-Host "5. Upload: pio run --target upload --upload-port COM3"
    Write-Host ""
    
    # Ask to open folder
    $openFolder = Read-Host "🔧 Open bot folder in Explorer? (y/N)"
    if ($openFolder.ToLower() -eq 'y') {
        Invoke-Item $BotDir
    }
    
} catch {
    Write-Host "❌ Error creating bot: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

Write-Host "🚀 Bot '$BotName' is ready for development!" -ForegroundColor Green