/*
 * =====================================================================================
 * Generic ESP32 Bot Template
 * =====================================================================================
 * 
 * Pre-configured with ESP-NOW and MCP communication setup.
 * Ready for sensor integration and customization!
 * 
 * Features included:
 * ✅ WiFi connection with auto-retry
 * ✅ ESP-NOW peer-to-peer communication
 * ✅ MCP server integration with mDNS discovery
 * ✅ OTA firmware updates
 * ✅ JSON status reporting
 * ✅ Automatic heartbeat and status sharing
 * ✅ WebSocket-style real-time communication
 * 
 * To customize for your bot:
 * 1. Update config.h with your bot name and WiFi credentials
 * 2. Add your sensor initialization in setupSensors()
 * 3. Add your sensor reading logic in readSensors()
 * 4. Add your bot-specific tasks in performBotTasks()
 * 5. Customize status payload in createStatusPayload()
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <HTTPUpdate.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include "config.h"

// =====================================================================================
// FORWARD DECLARATIONS - Core Communication Functions
// =====================================================================================
bool discoverMCPServer();
void initializeWiFi();
void initializeESPNow();
void initializeOTA();
void sendStatusToMCP();
void checkForFirmwareUpdate();
void onESPNowReceive(const uint8_t* mac, const uint8_t* incomingData, int len);
void onESPNowSend(const uint8_t* mac_addr, esp_now_send_status_t status);
void sendESPNowActivity(const String& senderMac, const String& receiverMac, 
                       const String& messageType, const String& payload);
void sendESPNowMessage(const uint8_t* peerMac, const String& messageType, const String& payload);
void addESPNowPeer(const uint8_t* peerMac);
void sendHeartbeatToAllPeers();
void sendStatusToAllPeers();
void respondToPeerMessage(const uint8_t* peerMac, const String& responseType);

// ⭐ CUSTOMIZE THESE: Add your sensor function declarations here
void setupSensors();
void readSensors();
void performBotTasks();
String createStatusPayload();

// =====================================================================================
// COMMUNICATION CONFIGURATION - Pre-configured
// =====================================================================================
String mcpServerIP = "";
int mcpServerPort = 0;
const char* mcpStatusEndpoint = "/api/bot/status";
const char* mcpFirmwareEndpoint = "/api/firmware/latest";
const char* mcpESPNowEndpoint = "/api/esp-now/message";

// Bot Configuration
String botID;
unsigned long lastStatusUpdate = 0;
const unsigned long statusUpdateInterval = 10000; // 10 seconds
unsigned long lastFirmwareCheck = 0;
const unsigned long firmwareCheckInterval = 300000; // 5 minutes
const unsigned long wifiRetryInterval = 30000; // 30 seconds

// ESP-NOW Message Structure (optimized for 250-byte limit)
typedef struct {
    char sender_id[24];     // Bot identifier
    char message_type[12];  // Message type
    char payload[128];      // Data payload
    uint32_t timestamp;     // Timestamp
} esp_now_message_t;

// Bot Status Structure
struct BotStatus {
    String status;
    float batteryLevel;
    int wifiSignal;
    unsigned long uptime;
    struct {
        float lat;
        float lng;
    } location;
    
    // ⭐ CUSTOMIZE THIS: Add your sensor data fields
    // Example:
    // float temperature;
    // float humidity;
    // int lightLevel;
    // bool motionDetected;
} botStatus;

// Timing variables for periodic tasks
unsigned long lastHeartbeat = 0;
unsigned long lastStatusShare = 0;
unsigned long lastSensorRead = 0;

// =====================================================================================
// MAIN SETUP FUNCTION
// =====================================================================================
void setup() {
    Serial.begin(115200);
    Serial.println("🤖 Generic ESP32 Bot Starting...");
    
    // Initialize Bot ID
    if (strlen(BOT_CUSTOM_NAME) > 0) {
        botID = String(BOT_CUSTOM_NAME);
        Serial.printf("Using custom bot name: %s\n", BOT_CUSTOM_NAME);
    } else {
        botID = "ESP32_Bot_" + String(ESP.getEfuseMac(), HEX);
        Serial.printf("Using auto-generated bot ID: %s\n", botID.c_str());
    }
    
    // Initialize bot status
    botStatus.status = "starting";
    botStatus.batteryLevel = 100.0;
    botStatus.uptime = 0;
    
    // Core communication setup
    initializeWiFi();
    initializeESPNow();
    
    // Discover MCP server
    if (discoverMCPServer()) {
        Serial.println("✅ MCP server discovered successfully");
    } else {
        Serial.println("⚠️ Using fallback MCP server configuration");
    }
    
    // Initialize OTA updates
    initializeOTA();
    
    // ⭐ CUSTOMIZE THIS: Initialize your sensors
    setupSensors();
    
    // Mark bot as ready
    botStatus.status = "active";
    Serial.println("🚀 Bot initialization complete");
    
    // Send initial status
    sendStatusToMCP();
}

// =====================================================================================
// MAIN LOOP FUNCTION
// =====================================================================================
void loop() {
    unsigned long currentTime = millis();
    
    // Update uptime
    botStatus.uptime = currentTime / 1000;
    
    // ⭐ CUSTOMIZE THIS: Read sensors periodically
    if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
        readSensors();
        lastSensorRead = currentTime;
    }
    
    // Send heartbeat to peers
    if (currentTime - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        sendHeartbeatToAllPeers();
        lastHeartbeat = currentTime;
    }
    
    // Share status with peers
    if (currentTime - lastStatusShare >= STATUS_SHARE_INTERVAL) {
        sendStatusToAllPeers();
        lastStatusShare = currentTime;
    }
    
    // Send status to MCP server
    if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
        sendStatusToMCP();
        lastStatusUpdate = currentTime;
    }
    
    // Check for firmware updates periodically
    if (currentTime - lastFirmwareCheck >= firmwareCheckInterval) {
        checkForFirmwareUpdate();
        lastFirmwareCheck = currentTime;
    }

    // ⭐ CUSTOMIZE THIS: Perform bot-specific tasks
    performBotTasks();

    // Handle OTA updates
    ArduinoOTA.handle();
    
    delay(100); // Small delay to prevent watchdog issues
}

// =====================================================================================
// SENSOR FUNCTIONS - CUSTOMIZE THESE FOR YOUR BOT
// =====================================================================================

void setupSensors() {
    Serial.println("🔧 Setting up sensors...");
    
    // ⭐ CUSTOMIZE THIS: Initialize your sensors here
    // Examples:
    
    // Built-in LED for status indication
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Built-in button for testing
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Example sensor initializations:
    // Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);  // I2C sensors
    // pinMode(SENSOR_PIN_1, INPUT);          // Digital sensor
    // analogReadResolution(12);              // 12-bit ADC resolution
    
    Serial.println("✅ Sensor setup complete");
}

void readSensors() {
    // ⭐ CUSTOMIZE THIS: Read your sensor values here
    
    // Update WiFi signal strength
    botStatus.wifiSignal = WiFi.RSSI();
    
    // Simulate battery level (replace with actual battery reading)
    botStatus.batteryLevel = max(80.0, botStatus.batteryLevel - 0.1);
    
    // Example sensor readings:
    // botStatus.temperature = readTemperature();
    // botStatus.humidity = readHumidity();
    // botStatus.lightLevel = analogRead(LIGHT_SENSOR_PIN);
    // botStatus.motionDetected = digitalRead(MOTION_SENSOR_PIN);
    
    // Status LED indication
    digitalWrite(LED_PIN, (millis() / 1000) % 2); // Blink every second
}

void performBotTasks() {
    // ⭐ CUSTOMIZE THIS: Add your bot-specific behavior here
    
    // Example tasks:
    // - Check button presses
    // - Control actuators based on sensor readings
    // - Implement state machines
    // - Handle alarms or alerts
    
    // Example: Button press detection
    static bool lastButtonState = HIGH;
    bool buttonState = digitalRead(BUTTON_PIN);
    
    if (buttonState != lastButtonState && buttonState == LOW) {
        Serial.println("🔘 Button pressed!");
        // Send special message to peers
        sendStatusToAllPeers();
    }
    lastButtonState = buttonState;
}

String createStatusPayload() {
    // ⭐ CUSTOMIZE THIS: Create JSON payload with your sensor data
    
    DynamicJsonDocument doc(256);
    doc["battery"] = botStatus.batteryLevel;
    doc["wifi_signal"] = botStatus.wifiSignal;
    doc["uptime"] = botStatus.uptime;
    doc["status"] = botStatus.status;
    
    // Add your custom sensor data:
    // doc["temperature"] = botStatus.temperature;
    // doc["humidity"] = botStatus.humidity;
    // doc["light_level"] = botStatus.lightLevel;
    // doc["motion"] = botStatus.motionDetected;
    
    String payload;
    serializeJson(doc, payload);
    return payload;
}

// =====================================================================================
// COMMUNICATION FUNCTIONS - Pre-configured, no need to modify
// =====================================================================================

void initializeWiFi() {
    Serial.println("📡 Scanning for WiFi networks...");
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    Serial.printf("Found %d networks:\n", n);
    for (int i = 0; i < n; ++i) {
        Serial.printf("%d: %s (%d dBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
                      WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Encrypted");
    }
    Serial.println();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.printf("Connecting to WiFi network: %s\n", WIFI_SSID);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        botStatus.wifiSignal = WiFi.RSSI();
    } else {
        Serial.println("\n❌ WiFi connection failed!");
        Serial.printf("WiFi status code: %d\n", WiFi.status());
    }
}

void initializeESPNow() {
    uint8_t ourMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, ourMac);
    Serial.printf("ESP-NOW MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  ourMac[0], ourMac[1], ourMac[2], ourMac[3], ourMac[4], ourMac[5]);

    Serial.printf("esp_now_message_t size: %d bytes (ESP-NOW max: 250 bytes)\n", sizeof(esp_now_message_t));

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error initializing ESP-NOW");
        return;
    }
    Serial.println("✅ ESP-NOW initialized");

    esp_now_register_recv_cb(onESPNowReceive);
    esp_now_register_send_cb(onESPNowSend);

    // Add known peers
    for (int i = 0; i < KNOWN_PEERS_COUNT; i++) {
        addESPNowPeer(KNOWN_PEERS[i].mac);
        Serial.printf("📡 Added peer: %s\n", KNOWN_PEERS[i].name);
    }
}

bool discoverMCPServer() {
    Serial.println("🔍 Discovering MCP server via mDNS...");
    
    if (!MDNS.begin(botID.c_str())) {
        Serial.println("❌ Error setting up mDNS responder!");
        mcpServerIP = MCP_SERVER_IP_FALLBACK;
        mcpServerPort = 8081;
        return false;
    }

    int n = MDNS.queryService("mcp-server", "tcp");
    if (n == 0) {
        Serial.println("⚠️ mDNS: No MCP server found via discovery.");
        Serial.printf("mDNS: Using fallback IP: %s\n", MCP_SERVER_IP_FALLBACK);
        mcpServerIP = MCP_SERVER_IP_FALLBACK;
        mcpServerPort = 8081;
        return false;
    } else {
        mcpServerIP = MDNS.IP(0).toString();
        mcpServerPort = MDNS.port(0);
        Serial.printf("✅ MCP server found at: %s:%d\n", mcpServerIP.c_str(), mcpServerPort);
        return true;
    }
}

void initializeOTA() {
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.setHostname(botID.c_str());
    
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("🔄 Starting OTA update (" + type + ")");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\n✅ OTA update completed");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("❌ OTA Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    
    ArduinoOTA.begin();
    Serial.printf("✅ OTA Initialized. Ready for updates.\n");
}

void sendStatusToMCP() {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        return;
    }

    HTTPClient http;
    String url = "http://" + mcpServerIP + ":" + String(mcpServerPort) + mcpStatusEndpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    if (strlen(MCP_API_KEY) > 0) {
        http.addHeader("X-API-Key", MCP_API_KEY);
    }

    DynamicJsonDocument doc(512);
    doc["bot_id"] = botID;
    doc["timestamp"] = "2025-11-01T" + String(millis() / 1000);
    doc["status"] = botStatus.status;
    doc["battery_level"] = botStatus.batteryLevel;
    doc["wifi_signal"] = botStatus.wifiSignal;
    doc["uptime_seconds"] = botStatus.uptime;

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac_address"] = macStr;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
        Serial.printf("Status sent to MCP: %d\n", httpResponseCode);
    } else {
        Serial.printf("Error sending status: %d\n", httpResponseCode);
    }

    http.end();
}

void addESPNowPeer(const uint8_t* peerMac) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.printf("❌ Failed to add ESP-NOW peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    } else {
        Serial.printf("✅ ESP-NOW peer added successfully: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    }
}

void sendESPNowMessage(const uint8_t* peerMac, const String& messageType, const String& payload) {
    esp_now_message_t message;
    
    strncpy(message.sender_id, botID.c_str(), sizeof(message.sender_id) - 1);
    message.sender_id[sizeof(message.sender_id) - 1] = '\0';
    
    strncpy(message.message_type, messageType.c_str(), sizeof(message.message_type) - 1);
    message.message_type[sizeof(message.message_type) - 1] = '\0';
    
    strncpy(message.payload, payload.c_str(), sizeof(message.payload) - 1);
    message.payload[sizeof(message.payload) - 1] = '\0';
    
    message.timestamp = millis();

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);

    Serial.printf("📤 Sending ESP-NOW message to %s: type=%s, payload=%s\n", 
                  macStr, messageType.c_str(), payload.c_str());
    Serial.printf("Message size: %d bytes\n", sizeof(message));

    esp_err_t result = esp_now_send(peerMac, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        Serial.println("✅ ESP-NOW message sent successfully");
    } else {
        Serial.printf("❌ Error sending ESP-NOW message: %s (0x%x)\n", esp_err_to_name(result), result);
    }
}

void sendHeartbeatToAllPeers() {
    Serial.println("💓 Sending ESP-NOW heartbeat to all peers...");
    for (int i = 0; i < KNOWN_PEERS_COUNT; i++) {
        Serial.printf("Sending heartbeat to peer: %s\n", KNOWN_PEERS[i].name);
        sendESPNowMessage(KNOWN_PEERS[i].mac, "heartbeat", botID);
    }
}

void sendStatusToAllPeers() {
    Serial.println("📊 Sharing status with all peers...");
    String statusPayload = createStatusPayload();
    
    for (int i = 0; i < KNOWN_PEERS_COUNT; i++) {
        Serial.printf("📊 Sending status to peer: %s\n", KNOWN_PEERS[i].name);
        sendESPNowMessage(KNOWN_PEERS[i].mac, "status", statusPayload);
    }
}

void respondToPeerMessage(const uint8_t* peerMac, const String& responseType) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    
    Serial.printf("📤 Responding to %s with: %s\n", macStr, responseType.c_str());
    
    String responsePayload = botID + "_response";
    sendESPNowMessage(peerMac, responseType, responsePayload);
    
    // Report this ESP-NOW activity to MCP
    sendESPNowActivity(String(macStr), "self", responseType, responsePayload);
}

void onESPNowReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    esp_now_message_t* message = (esp_now_message_t*)incomingData;
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    Serial.printf("🎯 ESP-NOW RECEIVED from %s [%s]: %s\n", 
                  message->sender_id, message->message_type, message->payload);
    
    String msgType = String(message->message_type);
    String payload = String(message->payload);
    
    if (msgType == "heartbeat") {
        Serial.printf("💓 Heartbeat from %s\n", message->sender_id);
        respondToPeerMessage(mac, "heartbeat_ack");
    } else if (msgType == "status") {
        Serial.printf("📊 Status update from %s: %s\n", message->sender_id, payload.c_str());
        respondToPeerMessage(mac, "status_ack");
    } else if (msgType.endsWith("_ack")) {
        Serial.printf("✅ Acknowledgment from %s: %s\n", message->sender_id, msgType.c_str());
    } else {
        Serial.printf("❓ Unknown message type: %s\n", msgType.c_str());
    }
    
    // Report this ESP-NOW activity to MCP
    sendESPNowActivity(String(macStr), "self", String(message->message_type), String(message->payload));
}

void onESPNowSend(const uint8_t* mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    Serial.printf("ESP-NOW send to %s: %s\n", macStr, status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void sendESPNowActivity(const String& senderMac, const String& receiverMac, 
                       const String& messageType, const String& payload) {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        return;
    }
    
    HTTPClient http;
    String url = "http://" + String(mcpServerIP) + ":" + String(mcpServerPort) + mcpESPNowEndpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    if (strlen(MCP_API_KEY) > 0) {
        http.addHeader("X-API-Key", MCP_API_KEY);
    }
    
    DynamicJsonDocument doc(512);
    doc["sender_mac"] = senderMac;
    doc["receiver_mac"] = receiverMac;
    doc["message_type"] = messageType;
    
    JsonObject payloadObj = doc.createNestedObject("payload");
    payloadObj["data"] = payload;
    payloadObj["reported_by"] = botID;
    
    String jsonPayload;
    serializeJson(doc, jsonPayload);
    
    http.POST(jsonPayload);
    http.end();
}

void checkForFirmwareUpdate() {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        return;
    }

    HTTPClient http;
    String url = "http://" + mcpServerIP + ":" + String(mcpServerPort) + mcpFirmwareEndpoint;
    http.begin(url);

    if (strlen(MCP_API_KEY) > 0) {
        http.addHeader("X-API-Key", MCP_API_KEY);
    }

    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        float latestVersion = doc["version"];
        String downloadUrl = doc["download_url"];

        if (latestVersion > FIRMWARE_VERSION) {
            Serial.printf("🔄 Firmware update available: v%.1f -> v%.1f\n", FIRMWARE_VERSION, latestVersion);
            Serial.printf("Download URL: %s\n", downloadUrl.c_str());
            // Implement OTA update logic here if needed
        }
    }

    http.end();
}