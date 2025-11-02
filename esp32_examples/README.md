# ESP32 Bot Setup and Configuration Guide

This directory contains example Arduino sketches for ESP32-based bots that communicate with the Master Control Program.

## Hardware Requirements

- ESP32 development board (ESP32-WROOM-32 recommended)
- USB cable for programming
- Optional: Sensors (temperature, humidity, light, etc.)
- Optional: Battery pack for mobile operation

## Software Requirements

- Arduino IDE 1.8.x or 2.x
- ESP32 Board Package
- Required Libraries:
  - ArduinoJson (6.x)
  - WiFi (included with ESP32 package)
  - ESP-NOW (included with ESP32 package)
  - ArduinoOTA (included with ESP32 package)

## Installation Steps

### 1. Install Arduino IDE

Download and install the Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Install ESP32 Board Package

1. Open Arduino IDE
2. Go to File → Preferences
3. Add this URL to "Additional Board Manager URLs":

   ```txt
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```

4. Go to Tools → Board → Board Manager
5. Search for "esp32" and install "esp32 by Espressif Systems"

### 3. Install Required Libraries

1. Go to Sketch → Include Library → Manage Libraries
2. Search and install:
   - ArduinoJson by Benoit Blanchon (version 6.x)

### 4. Configure the Examples

#### bot_wifi_client.ino

This sketch connects bots to your WiFi network and sends status updates to the MCP server.

**Configuration:**

Update the configuration variables in the sketch before uploading:

```cpp
// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";           // Replace with your WiFi network name
const char* password = "YOUR_WIFI_PASSWORD";   // Replace with your WiFi password
const char* mcpApiKey = "YOUR_SECRET_API_KEY_HERE"; // Replace with the key from your MCP server config
const float FIRMWARE_VERSION = 1.0; // Increment this for new versions
const char* otaPassword = "YOUR_OTA_UPDATE_PASSWORD"; // Set a secure password for OTA updates
```

**Features:**

- Automatic WiFi connection and reconnection
- Periodic status updates to MCP server
- ESP-NOW activity reporting
- Simulated sensor data (temperature, humidity, light)
- Automatic MCP server discovery via mDNS
- Battery level simulation
- Bot health monitoring

#### esp_now_example.ino

This sketch demonstrates bot-to-bot communication using ESP-NOW protocol.

**Configuration:**

```cpp
// Known peer MAC addresses (add your bot MAC addresses here)
uint8_t knownPeers[][6] = {
    {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}, // Replace with actual MAC addresses
    {0x24, 0x6F, 0x28, 0xDD, 0xEE, 0xFF}, // of your other ESP32 bots
};

bool isMaster = false; // Set to true for one bot to act as coordinator
```

**Features:**

- ESP-NOW mesh networking
- Heartbeat messages between bots
- Sensor data broadcasting
- Command and response system
- Emergency message relaying
- Network topology discovery

## Getting MAC Addresses

Each ESP32 has a unique MAC address. To find the MAC address of your ESP32:

1. Upload this simple sketch:

```cpp
#include "WiFi.h"
 
void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(WiFi.macAddress());
}
 
void loop(){}
```

1. Open Serial Monitor and note the MAC address
2. Add this MAC address to the `knownPeers` array in other bots

## Network Setup

### MCP Server Setup

1. Ensure your MCP server is running on your local network
2. The ESP32 bot will automatically discover the server's IP address.

### WiFi Configuration

1. All bots should connect to the same WiFi network as the MCP server
2. Ensure your router allows device-to-device communication
3. Consider setting up a dedicated IoT network for better security

## Programming the ESP32

1. Connect ESP32 to your computer via USB
2. Open the desired sketch in Arduino IDE
3. Select the correct board: Tools → Board → ESP32 Arduino → ESP32 Dev Module
4. Select the correct port: Tools → Port → (select your ESP32 port)
5. Configure these settings:
   - Upload Speed: 921600
   - CPU Frequency: 240MHz
   - Flash Size: 4MB
   - Partition Scheme: Default 4MB with spiffs

6. Click Upload

## Testing the Setup

### WiFi Client Test

1. Upload `bot_wifi_client.ino` to your ESP32
2. Open Serial Monitor (115200 baud)
3. Verify WiFi connection
4. Check MCP server logs for incoming bot status updates
5. View bot data in the MCP dashboard

### ESP-NOW Test

1. Upload `esp_now_example.ino` to multiple ESP32s
2. Update MAC addresses in each sketch
3. Power on all devices
4. Use Serial Monitor commands to test communication:
   - `status` - Show bot information
   - `heartbeat` - Send heartbeat to all peers
   - `sensor` - Broadcast sensor data
   - `peers` - Show connected peers

## Troubleshooting

### WiFi Connection Issues

- Verify SSID and password are correct
- Check if WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure router allows new device connections
- Try moving closer to the router

### ESP-NOW Issues

- Verify MAC addresses are correct (use colon format: AA:BB:CC:DD:EE:FF)
- Ensure all devices are on the same WiFi channel
- Check that ESP-NOW is initialized successfully
- Verify peer devices are powered on and in range

### MCP Server Connection Issues

- Verify MCP server IP address and port
- Check firewall settings on MCP server computer
- Ensure MCP server is running and accessible
- Test network connectivity with ping

## Advanced Configuration

### Power Management

For battery-powered bots, consider implementing deep sleep:

```cpp
// Sleep for 30 seconds
esp_sleep_enable_timer_wakeup(30 * 1000000); // microseconds
esp_deep_sleep_start();
```

### Security

For production deployments:

1. Enable ESP-NOW encryption
2. Use WPA2-Enterprise for WiFi
3. Implement message authentication
4. Use HTTPS for MCP communication

### Sensors

Add real sensors by connecting them to ESP32 pins and reading values:

```cpp
// Example: DHT22 temperature/humidity sensor
#include "DHT.h"
DHT dht(2, DHT22); // Pin 2, DHT22 type

void setup() {
    dht.begin();
}

float temperature = dht.readTemperature();
float humidity = dht.readHumidity();
```

### Custom Message Types

Extend the ESP-NOW communication with custom message types:

```cpp
// Add to handleIncomingMessage function
else if (messageType == "custom_command") {
    handleCustomCommand(senderMac, message);
}
```

## Monitoring and Debugging

### Serial Monitor Commands (esp_now_example.ino)

- `status` - Display bot information
- `peers` - Show connected peer devices
- `stats` - Display network statistics
- `heartbeat` - Manually send heartbeat
- `sensor` - Manually broadcast sensor data
- `send <MAC> <message>` - Send direct message to specific bot
- `help` - Show available commands

### Log Levels

Adjust logging verbosity by changing the Serial.print statements or implementing different log levels for production vs. development.

## Example Network Topology

```txt
Bot_1 (Master) ←→ ESP-NOW ←→ Bot_2
   ↓                          ↓
  WiFi                      WiFi
   ↓                          ↓
   ←→ Router/WiFi Network ←→
             ↓
      MCP Server (PC)
      Dashboard Interface
```

This setup allows:

- Bots communicate directly via ESP-NOW (no internet required)
- Bots report to MCP via WiFi when available
- MCP maintains complete network visibility
- Dashboard provides real-time monitoring
