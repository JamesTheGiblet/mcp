# 🤖 Generic ESP32 Bot Template

## 🚀 Ready-to-Use Bot Template with ESP-NOW & MCP Integration

This template provides a complete, pre-configured ESP32 bot setup with advanced communication features. Perfect for quickly creating new bots with custom sensors and behaviors!

## ✨ Pre-Configured Features

### 📡 **Communication Setup (Ready to Use)**

- ✅ **WiFi Connection** with auto-retry and network scanning
- ✅ **ESP-NOW Peer-to-Peer** communication with JSON payloads
- ✅ **MCP Server Integration** with mDNS auto-discovery
- ✅ **OTA Firmware Updates** with password protection
- ✅ **Real-time Status Reporting** to central dashboard
- ✅ **Automatic Heartbeat System** for health monitoring
- ✅ **WebSocket-style Communication** with acknowledgments

### 🔧 **Sensor Framework (Ready to Customize)**

- ✅ **Modular Sensor Integration** points
- ✅ **Periodic Reading System** with configurable intervals
- ✅ **JSON Status Payloads** for rich data sharing
- ✅ **Built-in Examples** (LED, button, simulated sensors)

## 🛠️ Quick Start Guide

### 1. **Copy the Template**

```bash
# Copy the entire generic_bot_template folder
cp -r generic_bot_template your_new_bot_name/
cd your_new_bot_name/
```

### 2. **Configure Your Bot**

Edit `src/config.h`:

```cpp
// ⭐ ESSENTIAL: Update these for your bot
const char* WIFI_SSID = "YOUR_WIFI_NETWORK";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* BOT_CUSTOM_NAME = "Your_Bot_Name";  // 🔧 Make this unique!

// ⭐ OPTIONAL: Add other bot MAC addresses to communicate with
const KnownPeer KNOWN_PEERS[] = {
    {{0x20, 0xe7, 0xc8, 0x59, 0x5c, 0xec}, "Wheelie_Bot"},
    {{0x20, 0xe7, 0xc8, 0x59, 0x5d, 0x08}, "Speedie_Bot"},
    // Add your other bots here
};

// ⭐ CUSTOMIZE: Add your sensor pin definitions
const int YOUR_SENSOR_PIN = 4;
const unsigned long YOUR_SENSOR_INTERVAL = 5000;
```

### 3. **Add Your Sensors**

Customize these functions in `src/main.cpp`:

```cpp
void setupSensors() {
    // ⭐ Initialize your sensors here
    pinMode(YOUR_SENSOR_PIN, INPUT);
    // Wire.begin(SDA_PIN, SCL_PIN);  // For I2C sensors
}

void readSensors() {
    // ⭐ Read your sensor values here
    // botStatus.temperature = readTemperatureSensor();
    // botStatus.humidity = readHumiditySensor();
}

String createStatusPayload() {
    // ⭐ Add your sensor data to JSON payload
    DynamicJsonDocument doc(256);
    doc["battery"] = botStatus.batteryLevel;
    doc["wifi_signal"] = botStatus.wifiSignal;
    // doc["temperature"] = botStatus.temperature;  // Add your data
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}
```

### 4. **Build and Upload**

```bash
# Build the firmware
pio run

# Upload to your ESP32
pio run --target upload --upload-port COM3  # Replace COM3 with your port

# Monitor serial output
pio device monitor --port COM3 --baud 115200
```

## 📋 Example Bot Implementations

### 🌡️ **Temperature Sensor Bot**

```cpp
// In config.h
const char* BOT_CUSTOM_NAME = "Temperature_Bot_01";
const int TEMP_SENSOR_PIN = 4;

// In setupSensors()
Wire.begin(21, 22);  // SDA, SCL pins
// Initialize your temperature sensor

// In readSensors()
botStatus.temperature = readDS18B20();
botStatus.humidity = readDHT22();

// In createStatusPayload()
doc["temperature"] = botStatus.temperature;
doc["humidity"] = botStatus.humidity;
```

### 📷 **Camera Security Bot**

```cpp
// In config.h
const char* BOT_CUSTOM_NAME = "Security_Cam_01";
const int PIR_SENSOR_PIN = 4;
const int LED_PIN = 2;

// In readSensors()
botStatus.motionDetected = digitalRead(PIR_SENSOR_PIN);
if (botStatus.motionDetected) {
    // Trigger camera capture
    captureAndSendImage();
}

// In createStatusPayload()
doc["motion_detected"] = botStatus.motionDetected;
doc["last_motion"] = lastMotionTime;
```

### 🌱 **Plant Monitor Bot**

```cpp
// In config.h
const char* BOT_CUSTOM_NAME = "Plant_Monitor_Alpha";
const int SOIL_MOISTURE_PIN = 35;
const int LIGHT_SENSOR_PIN = 34;

// In readSensors()
botStatus.soilMoisture = analogRead(SOIL_MOISTURE_PIN);
botStatus.lightLevel = analogRead(LIGHT_SENSOR_PIN);

// Smart watering logic
if (botStatus.soilMoisture < MOISTURE_THRESHOLD) {
    triggerWatering();
}
```

## 🔄 Communication Patterns

### **Automatic Features (No Code Needed)**

- 💓 **Heartbeat**: Sent every 30 seconds to all peers
- 📊 **Status Sharing**: Rich JSON data shared every 15 seconds
- 🔄 **MCP Reporting**: Status sent to central server every 10 seconds
- ✅ **Acknowledgments**: Automatic responses to peer messages

### **Custom Messaging**

```cpp
// Send custom message to specific peer
sendESPNowMessage(peerMac, "alert", "Motion detected!");

// Send to all peers
for (int i = 0; i < KNOWN_PEERS_COUNT; i++) {
    sendESPNowMessage(KNOWN_PEERS[i].mac, "sensor_data", sensorJson);
}
```

## 📊 Dashboard Integration

Your bot will automatically appear in the MCP dashboard with:

- 🤖 **Real-time Status** with custom sensor data
- 📡 **ESP-NOW Activity** with emoji indicators
- 📈 **Historical Data** and performance charts
- 🔔 **Health Monitoring** and alert system

## 🛡️ Security Features

- 🔐 **API Key Authentication** for MCP communication
- 🔒 **OTA Password Protection** for firmware updates
- 📱 **MAC Address Filtering** for ESP-NOW peers
- 🔍 **Activity Logging** for security auditing

## 📁 Project Structure

```txt
your_new_bot/
├── platformio.ini          # Build configuration
├── src/
│   ├── config.h            # ⭐ CUSTOMIZE: Bot settings
│   └── main.cpp            # ⭐ CUSTOMIZE: Sensor code
└── README.md               # This documentation
```

## 🔧 Customization Checklist

- [ ] Update `BOT_CUSTOM_NAME` in `config.h`
- [ ] Set WiFi credentials in `config.h`
- [ ] Add sensor pin definitions in `config.h`
- [ ] Implement `setupSensors()` for your sensors
- [ ] Implement `readSensors()` for data collection
- [ ] Customize `createStatusPayload()` for your data
- [ ] Add peer MAC addresses if needed
- [ ] Implement custom behavior in `performBotTasks()`

## 🚀 Ready Examples

Check out these working implementations:

- `../wheelie_bot/` - Movement tracking bot
- `../speedie_bot/` - Speed monitoring bot

## 💡 Tips for Success

1. **Start Simple**: Begin with built-in LED/button, then add sensors
2. **Test Communication**: Verify ESP-NOW messages in serial monitor
3. **Monitor Dashboard**: Check MCP dashboard for real-time data
4. **Use Unique Names**: Each bot needs a unique `BOT_CUSTOM_NAME`
5. **Check MAC Addresses**: Find them in serial output during startup

## 🎯 Your Bot is Ready

This template gives you everything needed for a sophisticated IoT bot. Just add your sensors and go! 🚀✨
