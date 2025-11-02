#!/usr/bin/env python3
"""
ESP32 Bot Generator Script
Quickly create new bots from the generic template
"""

import os
import shutil
import sys
from pathlib import Path

def create_new_bot():
    """Interactive bot creation from template"""
    
    print("🤖 ESP32 Bot Generator")
    print("=" * 50)
    
    # Get bot details from user
    bot_name = input("Enter your bot name (e.g., 'Temperature_Bot_01'): ").strip()
    if not bot_name:
        print("❌ Bot name is required!")
        return
    
    # Sanitize bot name for folder
    folder_name = bot_name.lower().replace(" ", "_").replace("-", "_")
    
    # Get project root (scripts folder is one level down from root)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    template_dir = project_root / "esp32_examples" / "generic_bot_template"
    new_bot_dir = project_root / "esp32_examples" / "bots" / folder_name
    
    # Check if template exists
    if not template_dir.exists():
        print(f"❌ Template not found at: {template_dir}")
        return
    
    # Check if bot already exists
    if new_bot_dir.exists():
        overwrite = input(f"⚠️ Bot '{folder_name}' already exists. Overwrite? (y/N): ").strip().lower()
        if overwrite != 'y':
            print("❌ Bot creation cancelled")
            return
        shutil.rmtree(new_bot_dir)
    
    # Copy template
    try:
        print(f"📁 Creating bot directory: {new_bot_dir}")
        shutil.copytree(template_dir, new_bot_dir)
        
        # Update config.h with bot name
        config_file = new_bot_dir / "src" / "config.h"
        if config_file.exists():
            with open(config_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Replace the generic bot name
            content = content.replace('const char* BOT_CUSTOM_NAME = "Generic_Bot";', 
                                    f'const char* BOT_CUSTOM_NAME = "{bot_name}";')
            
            with open(config_file, 'w', encoding='utf-8') as f:
                f.write(content)
            
            print(f"✅ Updated bot name to: {bot_name}")
        
        # Update README with bot-specific info
        readme_file = new_bot_dir / "README.md"
        if readme_file.exists():
            bot_readme_content = f"""# 🤖 {bot_name}

## Bot Configuration
- **Name**: `{bot_name}`
- **Template**: Generic ESP32 Bot Template
- **Created**: {Path(__file__).stat().st_mtime}

## Quick Start
1. Edit `src/config.h` with your WiFi credentials
2. Customize sensor code in `src/main.cpp`
3. Build and upload: `pio run --target upload`

## Customization Points
- [ ] Update WiFi settings in `config.h`
- [ ] Add sensor initialization in `setupSensors()`
- [ ] Implement sensor reading in `readSensors()`
- [ ] Customize status payload in `createStatusPayload()`
- [ ] Add bot behavior in `performBotTasks()`

## Communication Features
- ✅ ESP-NOW peer-to-peer messaging
- ✅ MCP server integration
- ✅ OTA firmware updates
- ✅ Real-time dashboard monitoring

Refer to the generic template README for detailed documentation.
"""
            with open(readme_file, 'w', encoding='utf-8') as f:
                f.write(bot_readme_content)
        
        print("\n🎉 Bot created successfully!")
        print(f"📁 Location: {new_bot_dir}")
        print("\n📋 Next Steps:")
        print(f"1. cd {new_bot_dir}")
        print("2. Edit src/config.h with your WiFi credentials")
        print("3. Customize sensor code in src/main.cpp")
        print("4. Build: pio run")
        print("5. Upload: pio run --target upload --upload-port COM3")
        
        # Ask if user wants to open the directory
        if sys.platform == "win32":
            open_folder = input("\n🔧 Open bot folder in Explorer? (y/N): ").strip().lower()
            if open_folder == 'y':
                os.startfile(new_bot_dir)
        
    except Exception as e:
        print(f"❌ Error creating bot: {e}")

def list_existing_bots():
    """List all existing bot projects"""
    script_dir = Path(__file__).parent
    esp32_dir = script_dir / "esp32_examples"
    
    if not esp32_dir.exists():
        print("❌ esp32_examples directory not found")
        return
    
    print("🤖 Existing Bot Projects:")
    print("=" * 40)
    
    bot_count = 0
    for item in esp32_dir.iterdir():
        if item.is_dir() and (item / "platformio.ini").exists():
            # Skip template
            if item.name == "generic_bot_template":
                continue
                
            config_file = item / "src" / "config.h"
            bot_name = "Unknown"
            
            if config_file.exists():
                try:
                    with open(config_file, 'r', encoding='utf-8') as f:
                        content = f.read()
                        for line in content.split('\n'):
                            if 'BOT_CUSTOM_NAME' in line and '=' in line:
                                bot_name = line.split('"')[1] if '"' in line else "Unknown"
                                break
                except:
                    pass
            
            print(f"📁 {item.name}")
            print(f"   Name: {bot_name}")
            print(f"   Path: {item}")
            print()
            bot_count += 1
    
    if bot_count == 0:
        print("No bot projects found (excluding template)")
    else:
        print(f"Total: {bot_count} bot projects")

def main():
    """Main menu"""
    while True:
        print("\n🤖 ESP32 Bot Manager")
        print("=" * 30)
        print("1. Create new bot from template")
        print("2. List existing bots") 
        print("3. Exit")
        
        choice = input("\nEnter your choice (1-3): ").strip()
        
        if choice == "1":
            create_new_bot()
        elif choice == "2":
            list_existing_bots()
        elif choice == "3":
            print("👋 Goodbye!")
            break
        else:
            print("❌ Invalid choice. Please enter 1, 2, or 3.")

if __name__ == "__main__":
    main()