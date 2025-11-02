#pragma once

/*
 * =====================================================================================
 * Generic Bot Configuration Template
 * =====================================================================================
 * 
 * This template provides pre-configured ESP-NOW and MCP communication setup.
 * Just customize the bot name and add your sensor-specific code!
 * 
 * IMPORTANT: Add this file to your .gitignore to keep your credentials safe!
 * 
 */

// -- WiFi Configuration --
// Replace with your WiFi network credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// -- MCP Server Configuration --
// If mDNS discovery fails, the bot will use this IP. Set to your MCP server's IP.
const char* MCP_SERVER_IP_FALLBACK = "192.168.1.130";

// -- Bot Identity Configuration --
// ⭐ CUSTOMIZE THIS: Set your unique bot name
const char* BOT_CUSTOM_NAME = "Temperature_Sensor";  // 🔧 Change this for each new bot!
// Examples: "Sensor_Bot_Alpha", "Guard_Bot_01", "Weather_Station", "Camera_Bot"

// -- ESP-NOW Peer Configuration --
// ⭐ CUSTOMIZE THIS: Add MAC addresses of other bots you want to communicate with
// Find MAC addresses from serial output: "ESP-NOW MAC: XX:XX:XX:XX:XX:XX"
struct KnownPeer {
    uint8_t mac[6];
    const char* name;
};

// Add your known bot peers here (examples included):
const KnownPeer KNOWN_PEERS[] = {
    {{0x20, 0xe7, 0xc8, 0x59, 0x5c, 0xec}, "Wheelie_Bot"},  // Example: Wheelie Bot
    {{0x20, 0xe7, 0xc8, 0x59, 0x5d, 0x08}, "Speedie_Bot"},  // Example: Speedie Bot
    // 🔧 Add more peers here:
    // {{0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}, "Your_Bot_Name"},
};
const int KNOWN_PEERS_COUNT = sizeof(KNOWN_PEERS) / sizeof(KnownPeer);

// -- Security & Firmware Configuration --
const char* MCP_API_KEY = "api_secure_key_1234567890";
const char* OTA_PASSWORD = "ota_password_key_1234567890";
const float FIRMWARE_VERSION = 1.0;

// -- Sensor Configuration Section --
// ⭐ CUSTOMIZE THIS: Add your sensor-specific configurations here

// Example sensor pins (modify as needed):
const int LED_PIN = 2;              // Built-in LED
const int BUTTON_PIN = 0;           // Boot button
// const int SENSOR_PIN_1 = 4;      // GPIO4 for sensor 1
// const int SENSOR_PIN_2 = 5;      // GPIO5 for sensor 2
// const int I2C_SDA_PIN = 21;      // I2C SDA pin
// const int I2C_SCL_PIN = 22;      // I2C SCL pin

// Example sensor intervals (modify as needed):
const unsigned long SENSOR_READ_INTERVAL = 5000;    // Read sensors every 5 seconds
const unsigned long HEARTBEAT_INTERVAL = 30000;     // Send heartbeat every 30 seconds
const unsigned long STATUS_SHARE_INTERVAL = 15000;  // Share status every 15 seconds

// Example sensor thresholds (modify as needed):
// const float TEMPERATURE_THRESHOLD = 30.0;
// const float HUMIDITY_THRESHOLD = 80.0;
// const int LIGHT_THRESHOLD = 500;
