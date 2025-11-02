#pragma once

/*
 * =====================================================================================
 * WHEELIE SCOUT BOT - Advanced Surveillance Configuration TEMPLATE
 * =====================================================================================
 * 
 * Hardware Platform: ESP32 Type C CH340 Development Board
 * - ESP-WROOM-32 module with 2.4GHz dual-mode WiFi+Bluetooth
 * - CH340 USB-to-Serial chip for stable communication
 * - USB Type-C interface for reliable power and programming
 * - 40nm low power technology, rich peripherals
 * - Supports automatic download without manual reset
 * - Compatible with Windows development environment
 * 
 * SECURITY NOTICE: This is a template file for public GitHub repository.
 * Copy this file to config.h and fill in your actual credentials.
 * 
 * SETUP INSTRUCTIONS:
 * 1. Copy config_template.h to config.h
 * 2. Replace placeholder values with your actual WiFi credentials
 * 3. Never commit config.h to GitHub (it's in .gitignore)
 * 
 */

// -- WiFi Configuration --
// Replace these with your actual WiFi credentials
const char* WIFI_SSID = "YOUR_WIFI_SSID_HERE";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD_HERE";

// -- MCP Server Fallback Configuration --
// If mDNS discovery fails, the bot will use this IP. Set to your server's static IP.
const char* MCP_SERVER_IP_FALLBACK = "192.168.1.100";  // Update with your server IP

// -- Bot Identity Configuration --
// Set a custom bot name. Leave empty "" to use auto-generated name based on MAC address
const char* BOT_CUSTOM_NAME = "Wheelie_Scout_Bot";  // Scout Surveillance Bot

// -- REAL HARDWARE CONFIGURATION (ESP32 38-Pin + L298N Motor Driver) --
// Power: 2x 3.7V batteries in series (7.4V) -> Buck converter -> 5V/3.3V
// Motor Driver: Compact L298N Module (4-pin version: IN1, IN2, IN3, IN4 only)
// This smaller L298N has built-in speed control - no separate ENA/ENB pins

// L298N Motor Driver Pins (connects to ESP32)
// Left Motor (Motor A)
const int MOTOR_LEFT_IN1 = 23;                                // IN1 - Left motor direction/speed
const int MOTOR_LEFT_IN2 = 22;                           // IN2 - Left motor direction/speed
// Right Motor (Motor B)  
const int MOTOR_RIGHT_IN3 = 19;                         // IN3 - Right motor direction/speed
const int MOTOR_RIGHT_IN4 = 18;                     // IN4 - Right motor direction/speed

// Sensor Pins
const int RCWL_5016_PIN = 26;      // RCWL-5016 motion/interaction sensor (digital)
const int SOUND_SENSOR_PIN = 17;   // TS-YM-115 sound detection sensor (digital)

// I2C Sensor Configuration (updated for compact L298N)
const int I2C_SDA = 21;            // I2C SDA pin for sensors (now available)
const int I2C_SCL = 25;            // I2C SCL pin for sensors (moved from 22)
// VL53L0X ToF Distance Sensor (I2C address: 0x29)
// MPU-9250 9-axis IMU (I2C address: 0x68)

// Indication & Alert Pins
// RGB LED Configuration (Red, Green, Blue pins + common ground)
const int LED_RED_PIN = 2;         // RGB LED - Red channel
const int LED_GREEN_PIN = 4;       // RGB LED - Green channel  
const int LED_BLUE_PIN = 5;        // RGB LED - Blue channel
// Note: RGB LED common ground connects to ESP32 GND
const int BUZZER_PIN = 12;         // Standard buzzer for audio alerts (GPIO 12 supports PWM)

// Additional GPIO for future expansion
const int SPARE_GPIO_1 = 33;       // Available for additional sensors
const int SPARE_GPIO_2 = 34;       // Available for additional sensors
const int SPARE_GPIO_3 = 36;       // Input only - for analog sensors
const int SPARE_GPIO_4 = 39;       // Input only - for analog sensors

// Power Management & Battery Monitoring
const float BATTERY_VOLTAGE_MIN = 6.4;  // Minimum safe voltage (2 x 3.2V)
const float BATTERY_VOLTAGE_MAX = 8.4;  // Maximum voltage (2 x 4.2V)
const int BATTERY_MONITOR_PIN = 36;     // ADC pin for voltage monitoring

// Voltage Divider Configuration (for battery monitoring)
// R1 (high side): 10kΩ, R2 (low side): 4.7kΩ
// Divider ratio: 4.7kΩ / (10kΩ + 4.7kΩ) = 0.3197
// Max battery 8.4V → ADC sees: 8.4V × 0.3197 = 2.69V (safe for 3.3V ADC)

// Scout Bot Movement Configuration
const int DEFAULT_SPEED = 180;     // Conservative speed for scout missions
const int TURN_SPEED = 140;        // Speed for turning maneuvers
const int STEALTH_SPEED = 100;     // Slow speed for stealth operations
const int MAX_SPEED = 220;         // Maximum safe speed
const int MIN_SPEED = 80;          // Minimum effective speed

// Scout Mission Parameters
const unsigned long SCOUT_PATROL_INTERVAL = 45000;    // 45 seconds scout patrol
const unsigned long MOTION_SCAN_INTERVAL = 500;       // 500ms motion detection
const unsigned long SOUND_SCAN_INTERVAL = 200;        // 200ms sound monitoring
const unsigned long DISTANCE_SCAN_INTERVAL = 100;     // 100ms ToF distance measurement
const unsigned long IMU_SCAN_INTERVAL = 50;           // 50ms IMU readings (20Hz)
const unsigned long ALERT_DURATION = 2000;            // 2 second alert duration

// Sensor Thresholds
const int OBSTACLE_DISTANCE_MM = 200;      // 20cm obstacle detection (VL53L0X in mm)
const int CLOSE_APPROACH_MM = 100;         // 10cm close approach warning
const float TILT_THRESHOLD = 30.0;         // 30 degree tilt detection
const float ACCELERATION_THRESHOLD = 2.0;  // 2G acceleration detection
const float ROTATION_THRESHOLD = 90.0;     // 90 deg/s rotation detection

// Alert & Response Configuration
const int MOTION_DETECTED_PAUSE = 3000;    // Pause 3s when motion detected
const int SOUND_DETECTED_PAUSE = 2000;     // Pause 2s when sound detected
const int ALERT_BUZZER_FREQ = 1000;        // 1kHz buzzer frequency
const int STATUS_BLINK_INTERVAL = 1000;    // LED blink rate

// Mission Types
enum ScoutMission {
    MISSION_PATROL,      // Normal patrol mode
    MISSION_STEALTH,     // Silent reconnaissance 
    MISSION_ALERT,       // High alert surveillance
    MISSION_INVESTIGATE, // Investigate detected activity
    MISSION_STANDBY      // Standby/idle mode
};

// -- Security & Firmware Configuration --
// Replace with your own secure keys
const char* MCP_API_KEY = "CHANGE_THIS_API_KEY_FOR_SECURITY";
const char* OTA_PASSWORD = "CHANGE_THIS_OTA_PASSWORD_FOR_SECURITY";
const float FIRMWARE_VERSION = 1.0;