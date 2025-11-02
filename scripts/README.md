# Scripts Folder

This folder contains automation scripts for the MCP project.

## Bot Creation Scripts

### Quick Start

From the project root, simply run:

```bash
create_bot.bat "Your_Bot_Name"
```

### Available Scripts

- **`create_bot.bat`** - Windows batch script for creating new bots
- **`create_bot.ps1`** - PowerShell version (alternative)
- **`create_bot.py`** - Python version (cross-platform)

### What These Scripts Do

1. Copy the `generic_bot_template`
2. Create new bot in `esp32_examples/bots/` folder
3. Update bot name in configuration files
4. Ready for customization and building

### Usage Examples

```bash
# Create a temperature monitoring bot
create_bot.bat "Temperature_Monitor"

# Create a security camera bot  
create_bot.bat "Security_Cam_01"

# Create a environmental sensor
create_bot.bat "Environment_Station"
```

All new bots are created in the `esp32_examples/bots/` directory and are ready to build with PlatformIO.
