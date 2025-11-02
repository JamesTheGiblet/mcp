#pragma once

/*
 * =====================================================================================
 * Bot Configuration - Keep Your Secrets Here!
 * =====================================================================================
 * 
 * This file contains all the user-configurable settings for the bot.
 * 
 * IMPORTANT: Add this file to your .gitignore to keep your credentials safe!
 * 
 */

// -- WiFi Configuration --
const char* WIFI_SSID = "TNCAP7550FE 2.4";
const char* WIFI_PASSWORD = "906425FD47";

// -- MCP Server Fallback Configuration --
// If mDNS discovery fails, the bot will use this IP. Set to your server's static IP.
const char* MCP_SERVER_IP_FALLBACK = "192.168.1.130";

// -- Bot Identity Configuration --
// Set a custom bot name. Leave empty "" to use auto-generated name based on MAC address
const char* BOT_CUSTOM_NAME = "Speedie_Bot(2)";  // Example: "Scout_Bot", "Guard_Bot", "Sensor_Bot_Alpha"

// -- Security & Firmware Configuration --
const char* MCP_API_KEY = "api_secure_key_1234567890";
const char* OTA_PASSWORD = "ota_password_key_1234567890";
const float FIRMWARE_VERSION = 1.0;