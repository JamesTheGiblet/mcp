# Master Control Program (MCP) - Project Structure

## 📁 **NEW ORGANIZED PROJECT STRUCTURE**

```txt
mcp/
├── 📄 .gitignore                    # Git ignore patterns
├── 📄 README.md                     # Main project documentation
├── 📄 SETUP.md                      # Setup instructions
├── 📄 requirements.txt              # Python dependencies
├── 📄 create_bot.bat               # 🆕 Bot creation launcher (redirects to scripts/)
├── 📄 mcp_server.py                # Main MCP server application
├── 📄 bot_manager.py               # Bot management logic
├── 📄 database.py                  # Database operations
├── 📁 config/                      # Configuration files
│   └── 📄 config.json             # Server configuration
├── 📁 data/                        # Runtime data
│   ├── 📄 mcp.db                  # SQLite database
│   └── 📁 logs/                   # Log files
│       └── 📄 mcp.log             # Server logs
├── 📁 docs/                       # 🆕 Documentation folder
│   ├── 📄 PROJECT_STRUCTURE.md   # This file (moved from root)
│   ├── 📄 TEMPLATE_SYSTEM_SUMMARY.md # Template system documentation
│   └── 📄 GENERIC_BOT_GUIDE.md   # Bot creation guide
├── 📁 scripts/                    # 🆕 Automation scripts
│   ├── 📄 create_bot.bat         # Windows bot creation script
│   ├── 📄 create_bot.ps1         # PowerShell bot creation script
│   └── 📄 create_bot.py          # Python bot creation script
├── 📁 web_dashboard/              # Web interface
│   ├── 📄 index.html             # Dashboard HTML
│   └── 📁 static/                # Static assets
│       ├── 📁 css/               # Stylesheets
│       └── 📁 js/                # JavaScript files
└── 📁 esp32_examples/             # ESP32 firmware projects
    ├── 📄 README.md              # ESP32 setup guide
    ├── 📄 BUILD_FIX_SUMMARY.md  # Build troubleshooting guide
    ├── 📁 generic_bot_template/  # 🆕 Ready-to-use bot template
    │   ├── 📄 README.md         # Template documentation
    │   ├── 📄 platformio.ini    # PlatformIO config
    │   └── 📁 src/              # Template source code
    │       ├── 📄 main.cpp      # Pre-configured ESP-NOW & MCP
    │       └── 📄 config.h      # Customizable configuration
    ├── 📁 bots/                 # 🆕 Production bots
    │   ├── 📁 wheelie_bot/      # Wheelie Bot firmware
    │   │   ├── 📄 platformio.ini # PlatformIO config
    │   │   └── 📁 src/          # Source code
    │   ├── � speedie_bot/      # Speedie Bot firmware
    │   │   ├── 📄 platformio.ini # PlatformIO config
    │   │   └── 📁 src/          # Source code
    │   └── 📁 Temperature_Sensor/ # Test bot created from template
    │       ├── 📄 platformio.ini # PlatformIO config
    │       └── 📁 src/          # Source code
    └── � examples/             # 🆕 Example/reference projects
        ├── 📁 bot_wifi_client/  # WiFi client example
        │   ├── 📄 platformio.ini # PlatformIO config
        │   └── 📁 src/          # Source code
        └── 📁 esp_now_example/  # ESP-NOW example
            ├── 📄 platformio.ini # PlatformIO config
            └── 📁 src/          # Source code
```

## 🧹 Cleaned Up Files

### ✅ Removed

- `__pycache__/` - Python bytecode cache
- `*.pio/` - PlatformIO build directories  
- `.vscode/` - VS Code workspace settings
- `*.ino` - Duplicate Arduino sketch files
- `config_guard.h` / `config_scout.h` - Unused config files
- `data/firmware/` - Empty firmware directory

### ✅ Added

- `.gitignore` - Comprehensive ignore patterns
- This structure documentation
- `📁 generic_bot_template/` - **🆕 Ready-to-use ESP32 bot template**
- `📄 create_bot.py` - **🆕 Automated bot creation script**

## 🚀 Active Components

### Current Bot Network

- **Wheelie Bot** (MAC: 20:E7:C8:59:5C:EC) - Active ✅
- **Speedie Bot** (MAC: 20:E7:C8:59:5D:08) - Active ✅

### Communication Features

- **ESP-NOW**: Peer-to-peer messaging with JSON payloads
- **HTTP/WiFi**: Status reporting to MCP server
- **WebSocket**: Real-time dashboard updates
- **Enhanced Monitoring**: Emoji indicators and payload details

## 🤖 Quick Bot Creation

### **Create New Bot from Template**

```bash
# Use the automated script
python create_bot.py

# Or manually copy template
cp -r esp32_examples/generic_bot_template/ esp32_examples/my_new_bot/
cd esp32_examples/my_new_bot/
# Edit src/config.h with your bot name and WiFi credentials
```

### **Template Features** ⭐

- ✅ **Pre-configured ESP-NOW** communication
- ✅ **MCP server integration** with mDNS discovery
- ✅ **OTA firmware updates** ready
- ✅ **Sensor framework** with customization points
- ✅ **JSON status payloads** for rich data sharing
- ✅ **Automatic heartbeat** and health monitoring

## 📋 Build Commands

```bash
# Clean and build all ESP32 projects
cd esp32_examples/wheelie_bot && pio run --target clean && pio run
cd esp32_examples/speedie_bot && pio run --target clean && pio run

# Upload firmware
pio run --target upload --upload-port COMx

# Start MCP server
python mcp_server.py
```

Your project is now clean and well-organized! 🎯
