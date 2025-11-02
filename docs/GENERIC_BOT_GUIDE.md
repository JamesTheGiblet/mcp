# 🤖 ESP32 Generic Bot Template - Quick Guide

## 🚀 What You Get

Your **Generic Bot Template** includes everything needed for sophisticated ESP32 bot development:

### ✅ **Pre-Configured Communication**

- **ESP-NOW**: Peer-to-peer messaging with automatic acknowledgments
- **WiFi**: Auto-connecting with network scanning and retry logic  
- **MCP Integration**: Central server reporting with mDNS discovery
- **OTA Updates**: Wireless firmware updates with password protection
- **JSON Payloads**: Rich data sharing between bots and server

### ✅ **Ready Sensor Framework**

- Modular sensor integration points
- Periodic reading system with configurable intervals
- Built-in examples (LED, button, simulated sensors)
- Customizable status payloads

### ✅ **Smart Features**

- Automatic heartbeat system for health monitoring
- Real-time dashboard integration with emoji indicators
- Activity logging and historical data
- Robust error handling and recovery

## 🛠️ Creating Your First Bot

### **Option 1: Quick Creation (Windows)**

```bash
# Use the batch script
.\create_bot.bat "Your_Bot_Name"

# Example:
.\create_bot.bat "Weather_Station_01"
.\create_bot.bat "Security_Camera_Bot"
.\create_bot.bat "Plant_Monitor_Alpha"
```

### **Option 2: Manual Creation**

```bash
# Copy template manually
cp -r esp32_examples/generic_bot_template/ esp32_examples/your_new_bot/
cd esp32_examples/your_new_bot/
```

## 🔧 Essential Customization

### **1. Update Configuration** (`src/config.h`)

```cpp
// ⭐ ESSENTIAL: Update these for your bot
const char* WIFI_SSID = "YOUR_WIFI_NETWORK";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";  
const char* BOT_CUSTOM_NAME = "Your_Unique_Bot_Name";

// ⭐ OPTIONAL: Add other bot MAC addresses for communication
const KnownPeer KNOWN_PEERS[] = {
    {{0x20, 0xe7, 0xc8, 0x59, 0x5c, 0xec}, "Wheelie_Bot"},
    {{0x20, 0xe7, 0xc8, 0x59, 0x5d, 0x08}, "Speedie_Bot"},
    // Add your other bots here
};
```

### **2. Add Your Sensors** (`src/main.cpp`)

```cpp
void setupSensors() {
    // ⭐ Initialize your sensors here
    pinMode(SENSOR_PIN, INPUT);
    // Wire.begin(SDA_PIN, SCL_PIN);  // For I2C sensors
}

void readSensors() {
    // ⭐ Read your sensor values here
    botStatus.temperature = readTemperatureSensor();
    botStatus.humidity = readHumiditySensor();
}

String createStatusPayload() {
    // ⭐ Add your sensor data to JSON
    DynamicJsonDocument doc(256);
    doc["battery"] = botStatus.batteryLevel;
    doc["temperature"] = botStatus.temperature;
    // Add more sensor data...
}
```

## 📋 Build & Deploy

```bash
# Navigate to your bot
cd esp32_examples/your_bot_name/

# Build firmware
pio run

# Upload to ESP32 (replace COM3 with your port)
pio run --target upload --upload-port COM3

# Monitor serial output
pio device monitor --port COM3 --baud 115200
```

## 🎯 Real-World Examples

### **Temperature Monitor Bot**

- Add DS18B20 or DHT22 sensor
- Monitor temperature/humidity
- Send alerts when thresholds exceeded
- Display on MCP dashboard

### **Security Camera Bot**  

- Add PIR motion sensor
- Trigger camera on motion
- Send ESP-NOW alerts to other bots
- Log security events

### **Plant Care Bot**

- Soil moisture sensor
- Light level monitoring  
- Automatic watering control
- Growth tracking data

## 📊 Dashboard Integration

Your bot automatically appears in the MCP dashboard with:

- 🤖 Real-time status with your custom sensor data
- 📡 ESP-NOW activity with emoji indicators (💓 heartbeat, 📊 status, ✅ ack)
- 📈 Historical charts and performance data
- 🔔 Health monitoring and alert system

## 🔄 Communication Patterns

### **Automatic (No Code Needed)**

- **Heartbeat**: Every 30 seconds to all peers
- **Status Sharing**: Every 15 seconds with sensor data
- **MCP Reporting**: Every 10 seconds to central server
- **Acknowledgments**: Automatic responses to messages

### **Custom Messaging**

```cpp
// Send alert to specific peer
sendESPNowMessage(peerMac, "alert", "Motion detected!");

// Broadcast to all peers  
for (int i = 0; i < KNOWN_PEERS_COUNT; i++) {
    sendESPNowMessage(KNOWN_PEERS[i].mac, "sensor_data", jsonData);
}
```

## 🛡️ Security & Reliability

- 🔐 **API Key Authentication** for MCP communication
- 🔒 **OTA Password Protection** for firmware updates
- 📱 **MAC Address Filtering** for ESP-NOW peers
- 🔍 **Activity Logging** for security auditing
- ♻️ **Auto-Recovery** from network issues

## 🎉 Your Bot Network is Ready

The template provides everything needed for professional IoT bot development:

- **Silent observation** of all bot communications  
- **Rich data exchange** with JSON payloads
- **Centralized monitoring** via web dashboard
- **Scalable architecture** for growing networks

Just add your sensors and customize the behavior - the communication infrastructure is ready! 🚀✨
