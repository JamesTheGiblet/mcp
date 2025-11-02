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
const char* BOT_CUSTOM_NAME = "Speedie_Bot(2)";  // High-Speed Reconnaissance Bot

// -- Hardware Pin Configuration (Speedie Bot Specific) --
// High-Performance Motor Control Pins
const int MOTOR_LEFT_PWM = 18;     // Left motor PWM (high power)
const int MOTOR_LEFT_DIR1 = 19;    // Left motor direction 1
const int MOTOR_LEFT_DIR2 = 21;    // Left motor direction 2
const int MOTOR_RIGHT_PWM = 22;    // Right motor PWM (high power)
const int MOTOR_RIGHT_DIR1 = 23;   // Right motor direction 1
const int MOTOR_RIGHT_DIR2 = 25;   // Right motor direction 2

// Speed and Navigation Sensors
const int SPEED_SENSOR_LEFT = 26;   // Left wheel speed sensor (hall effect)
const int SPEED_SENSOR_RIGHT = 27;  // Right wheel speed sensor (hall effect)
const int GPS_RX = 32;              // GPS module RX
const int GPS_TX = 33;              // GPS module TX
const int ACCEL_SDA = 34;           // Accelerometer I2C SDA
const int ACCEL_SCL = 35;           // Accelerometer I2C SCL
const int EMERGENCY_BUTTON = 4;     // Emergency stop button
const int LED_STATUS = 2;           // Status LED
const int BUZZER = 5;               // Alert buzzer

// High-Speed Movement Configuration
const int MAX_SPEED = 255;          // Maximum motor speed (full power)
const int CRUISE_SPEED = 220;       // Cruising speed for recon
const int EMERGENCY_SPEED = 255;    // Emergency response speed
const int TURN_SPEED = 180;         // Speed for high-speed turns
const int ACCELERATION_STEP = 10;   // Speed increase per step
const int DECELERATION_STEP = 15;   // Speed decrease per step (faster braking)

// Reconnaissance Configuration
const unsigned long RECON_INTERVAL = 5000;      // 5 seconds recon cycle (faster)
const unsigned long GPS_UPDATE_INTERVAL = 1000; // 1 second GPS updates
const unsigned long SPEED_CHECK_INTERVAL = 100; // 100ms speed monitoring
const float MAX_SAFE_SPEED = 2.0;               // Maximum safe speed (m/s)
const float EMERGENCY_DECEL_THRESHOLD = 1.5;    // Emergency deceleration trigger

// Mission Parameters
const int PATROL_RADIUS = 500;      // Patrol radius in meters
const int RAPID_RESPONSE_DISTANCE = 1000; // Rapid response range
const int ALERT_DISTANCE = 50;      // Distance to trigger alerts

// -- Security & Firmware Configuration --
const char* MCP_API_KEY = "api_secure_key_1234567890";
const char* OTA_PASSWORD = "ota_password_key_1234567890";
const float FIRMWARE_VERSION = 1.0;