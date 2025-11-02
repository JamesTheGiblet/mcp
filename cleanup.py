#!/usr/bin/env python3
"""
MCP Project Cleanup Script
Removes build artifacts and temporary files
"""

import os
import shutil
from pathlib import Path

def cleanup_project():
    """Clean up the MCP project directory"""
    project_root = Path(__file__).parent
    
    print("🧹 Cleaning up MCP project...")
    
    # Patterns to clean
    cleanup_patterns = [
        # Python cache
        "__pycache__",
        "*.pyc",
        "*.pyo",
        "*.pyd",
        ".Python",
        
        # PlatformIO build directories
        ".pio",
        ".pioenvs",
        ".piolibdeps",
        
        # IDE files
        ".vscode/c_cpp_properties.json",
        ".vscode/launch.json",
        
        # Temporary files
        "*.tmp",
        "*.temp",
        "*.log~",
    ]
    
    # Directories to clean
    directories_to_clean = [
        project_root / "__pycache__",
        project_root / "esp32_examples" / "wheelie_bot" / ".pio",
        project_root / "esp32_examples" / "speedie_bot" / ".pio",
        project_root / "esp32_examples" / "bot_wifi_client" / ".pio",
        project_root / "esp32_examples" / "esp_now_example" / ".pio",
    ]
    
    cleaned_count = 0
    
    # Clean directories
    for dir_path in directories_to_clean:
        if dir_path.exists():
            try:
                shutil.rmtree(dir_path)
                print(f"✅ Removed: {dir_path.relative_to(project_root)}")
                cleaned_count += 1
            except Exception as e:
                print(f"❌ Could not remove {dir_path}: {e}")
    
    # Clean individual files
    for pattern in cleanup_patterns:
        if "*" in pattern:
            # Handle glob patterns
            for file_path in project_root.rglob(pattern):
                if file_path.is_file():
                    try:
                        file_path.unlink()
                        print(f"✅ Removed: {file_path.relative_to(project_root)}")
                        cleaned_count += 1
                    except Exception as e:
                        print(f"❌ Could not remove {file_path}: {e}")
    
    print(f"\n🎉 Cleanup complete! Removed {cleaned_count} items.")
    print("📁 Project is now clean and ready for development.")

if __name__ == "__main__":
    cleanup_project()