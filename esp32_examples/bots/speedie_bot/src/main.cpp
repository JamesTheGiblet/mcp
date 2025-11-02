/*
 * ESP32 Bot WiFi Client - Speedie Bot
 * Connects to MCP server and sends status updates
 * Part of the Master Control Program IoT Bot Network
 * Firmware Version: 1.0
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
#include <SoftwareSerial.h>  // For GPS communication
#include <Wire.h>           // For I2C accelerometer
#include "config.h"

// Forward declarations - Communication Functions
bool discoverMCPServer();
void initializeWiFi();
void initializeESPNow();
void initializeOTA();
void sendStatusToMCP();
void performBotTasks();
void checkForFirmwareUpdate();
void onESPNowReceive(const uint8_t* mac, const uint8_t* incomingData, int len);
void onESPNowSend(const uint8_t* mac_addr, esp_now_send_status_t status);
void sendESPNowActivity(const String& senderMac, const String& receiverMac, 
                       const String& messageType, const String& payload);
void sendESPNowMessage(const uint8_t* peerMac, const String& messageType, const String& payload);
void addESPNowPeer(const uint8_t* peerMac);
void sendHeartbeatToAllPeers();
void sendStatusToAllPeers();
void respondToPeerMessage(const uint8_t* peerMac, const String& originalMessage);

// Forward declarations - Speedie Bot Specific Functions
void setupSpeedieBot();
void setupHighSpeedMotors();
void setupSpeedSensors();
void setupGPS();
void setupAccelerometer();
void reconMode();
void emergencyStop();
void rapidResponse(float targetLat, float targetLon);
void accelerateToSpeed(int targetSpeed);
void decelerateToSpeed(int targetSpeed);
void moveForwardHighSpeed(int speed = CRUISE_SPEED);
void emergencyBrake();
void highSpeedTurn(bool isLeft, int duration = 300);
void stopMotorsGradual();
void readSpeedSensors();
void readGPSData();
void readAccelerometerData();
void processHighSpeedCommand(const String& command);
void alertBuzzer(int duration = 500);
void checkEmergencyConditions();

// Speedie Bot Hardware Objects
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);

// Speedie Bot State Variables
struct SpeedieState {
    bool isInReconMode = false;
    bool emergencyActive = false;
    bool rapidResponseMode = false;
    float currentSpeedLeft = 0.0;    // Current left wheel speed (m/s)
    float currentSpeedRight = 0.0;   // Current right wheel speed (m/s)
    float averageSpeed = 0.0;        // Average bot speed
    float currentLat = 0.0;          // Current GPS latitude
    float currentLon = 0.0;          // Current GPS longitude
    float targetLat = 0.0;           // Target GPS latitude
    float targetLon = 0.0;           // Target GPS longitude
    float accelX = 0.0;              // Accelerometer X
    float accelY = 0.0;              // Accelerometer Y
    float accelZ = 0.0;              // Accelerometer Z
    int currentMotorSpeed = 0;       // Current motor PWM speed
    unsigned long lastReconAction = 0;
    unsigned long lastGPSUpdate = 0;
    unsigned long lastSpeedCheck = 0;
    String currentMission = "standby";
    String lastAlert = "none";
    volatile unsigned long leftPulseCount = 0;
    volatile unsigned long rightPulseCount = 0;
    unsigned long lastPulseTime = 0;
} speedieState;

// MCP Server Configuration (will be discovered via mDNS)
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

// ESP-NOW Configuration - Speedie Bot knows Wheelie's MAC
// Wheelie MAC: 20:e7:c8:59:5c:ec
uint8_t knownPeers[][6] = {
    {0x20, 0xe7, 0xc8, 0x59, 0x5c, 0xec}  // Wheelie Bot MAC
};

typedef struct {
    char sender_id[24];     // Reduced from 32
    char message_type[12];  // Reduced from 16  
    char payload[128];      // Reduced from 200 to 128
    uint32_t timestamp;
} esp_now_message_t;

// Status tracking
struct BotStatus {
    String status;
    float batteryLevel;
    int wifiSignal;
    unsigned long uptime;
    struct {
        float lat;
        float lng;
    } location;
    JsonObject sensorData;
} botStatus;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP32 Bot Starting...");
    
    // Initialize Bot ID based on config
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
    
    // Initialize WiFi
    initializeWiFi();
    
    // Initialize ESP-NOW
    initializeESPNow();
    
    // Add delay to ensure ESP-NOW is fully initialized
    delay(100);

    // Add specific known peers (skip broadcast and empty addresses)
    for (int i = 0; i < sizeof(knownPeers) / sizeof(knownPeers[0]); i++) {
        // Skip broadcast address and zero MAC addresses
        if (knownPeers[i][0] == 0xFF || 
            (knownPeers[i][0] == 0x00 && knownPeers[i][1] == 0x00 && 
             knownPeers[i][2] == 0x00 && knownPeers[i][3] == 0x00 && 
             knownPeers[i][4] == 0x00 && knownPeers[i][5] == 0x00)) {
            continue;
        }
        Serial.printf("Adding ESP-NOW peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      knownPeers[i][0], knownPeers[i][1], knownPeers[i][2],
                      knownPeers[i][3], knownPeers[i][4], knownPeers[i][5]);
        addESPNowPeer(knownPeers[i]);
    }
    
    // Discover MCP Server
    if (WiFi.status() == WL_CONNECTED) {
        discoverMCPServer();
        initializeOTA(); // Initialize OTA after WiFi is connected
    }

    botStatus.status = "active";
    Serial.println("Bot initialization complete");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Update uptime
    botStatus.uptime = currentTime / 1000;
    
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi disconnected, attempting reconnection...");
        initializeWiFi();
        // If reconnected, try to discover server again if we lost it
        if (WiFi.status() == WL_CONNECTED && mcpServerIP == "") {
            discoverMCPServer();
            initializeOTA(); // Re-initialize OTA if we reconnected
        }
    }
    
    // Send status update to MCP
    if (currentTime - lastStatusUpdate >= statusUpdateInterval) {
        sendStatusToMCP();
        lastStatusUpdate = currentTime;
    }
    
    // Check for firmware updates periodically
    if (currentTime - lastFirmwareCheck >= firmwareCheckInterval) {
        checkForFirmwareUpdate();
        lastFirmwareCheck = currentTime;
    }

    // Simulate some bot activities
    performBotTasks();

    // Handle any incoming OTA update requests
    ArduinoOTA.handle();
    
    delay(1000);
}

void initializeWiFi() {
    // Scan for available networks first
    Serial.println("Scanning for WiFi networks...");
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
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.printf("Signal strength: %d dBm\n", WiFi.RSSI());
        botStatus.wifiSignal = WiFi.RSSI();
    } else {
        Serial.println("\nWiFi connection failed!");
        Serial.printf("WiFi status code: %d\n", WiFi.status());
        botStatus.status = "wifi_error";
        mcpServerIP = ""; // Clear server IP if WiFi fails
        mcpServerPort = 0;
    }
}

bool discoverMCPServer() {
    Serial.println("Discovering MCP server via mDNS...");
    if (!MDNS.begin(botID.c_str())) {
        Serial.println("Error setting up MDNS responder!");
        return false;
    }

    int n = MDNS.queryService("mcp-server", "tcp");
    if (n == 0) {
        Serial.println("mDNS: No MCP server found via discovery.");
        // Try to use the fallback IP if discovery fails
        if (strlen(MCP_SERVER_IP_FALLBACK) > 0) {
            Serial.printf("mDNS: Using fallback IP: %s\n", MCP_SERVER_IP_FALLBACK);
            mcpServerIP = MCP_SERVER_IP_FALLBACK;
            mcpServerPort = 8081; // Default server port is 8081
            return true;
        } else {
            Serial.println("mDNS: No fallback IP configured. Will retry discovery later.");
            return false;
        }
    } else {
        mcpServerIP = MDNS.IP(0).toString();
        mcpServerPort = MDNS.port(0);
        Serial.print("MCP server found at: ");
        Serial.print(mcpServerIP);
        Serial.print(":");
        Serial.println(mcpServerPort);
        return true;
    }
    MDNS.end();
}

void initializeOTA() {
    // Set hostname for OTA identification
    ArduinoOTA.setHostname(botID.c_str());

    // Set a password for secure updates
    if (strlen(OTA_PASSWORD) > 0) {
        ArduinoOTA.setPassword(OTA_PASSWORD);
    }

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

    ArduinoOTA.begin();
    Serial.println("OTA Initialized. Ready for updates.");
}

void initializeESPNow() {
    // Get MAC address for ESP-NOW
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    Serial.printf("ESP-NOW MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Check message size limits
    Serial.printf("esp_now_message_t size: %d bytes (ESP-NOW max: 250 bytes)\n", sizeof(esp_now_message_t));
    if (sizeof(esp_now_message_t) > 250) {
        Serial.println("ERROR: Message structure too large for ESP-NOW!");
        return;
    }
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register callback for receiving data
    esp_now_register_recv_cb(onESPNowReceive);
    esp_now_register_send_cb(onESPNowSend);
    
    Serial.println("ESP-NOW initialized");
}

void sendStatusToMCP() {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        if (mcpServerIP == "") discoverMCPServer(); // Try to find it again
        Serial.println("Cannot send status - WiFi not connected or MCP server not found.");
        return;
    }
    
    HTTPClient http; // Create a new client for this request
    String url = "http://" + String(mcpServerIP) + ":" + String(mcpServerPort) + mcpStatusEndpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Add API Key header for security
    if (strlen(MCP_API_KEY) > 0) {
        http.addHeader("X-API-Key", MCP_API_KEY);
    }
    
    // Create JSON payload
    DynamicJsonDocument doc(1024);
    doc["bot_id"] = botID;
    doc["status"] = botStatus.status;
    doc["battery_level"] = botStatus.batteryLevel;
    doc["wifi_signal"] = WiFi.RSSI();
    doc["uptime_seconds"] = botStatus.uptime;
    
    // Add MAC address
    doc["mac_address"] = WiFi.macAddress();
    
    // Add location if available
    if (botStatus.location.lat != 0 || botStatus.location.lng != 0) {
        doc["location"]["lat"] = botStatus.location.lat;
        doc["location"]["lng"] = botStatus.location.lng;
    }
    
    // Add sensor data
    JsonObject sensorData = doc.createNestedObject("sensor_data");
    sensorData["temperature"] = random(200, 350) / 10.0; // Simulate temperature sensor
    sensorData["humidity"] = random(300, 800) / 10.0;    // Simulate humidity sensor
    sensorData["light_level"] = random(0, 1023);         // Simulate light sensor
    
    String payload;
    serializeJson(doc, payload);
    
    int httpResponseCode = http.POST(payload);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Status sent to MCP: " + String(httpResponseCode));
    } else {
        Serial.println("Error sending status: " + String(httpResponseCode));
    }
    
    http.end();
}

void sendESPNowActivity(const String& senderMac, const String& receiverMac, 
                       const String& messageType, const String& payload) {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        return;
    }
    
    HTTPClient http; // Create a new client for this request
    String url = "http://" + String(mcpServerIP) + ":" + String(mcpServerPort) + mcpESPNowEndpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Add API Key header for security
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

void onESPNowReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    esp_now_message_t* message = (esp_now_message_t*)incomingData;
    
    // Convert MAC to string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Get our MAC address
    uint8_t ourMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, ourMac);
    char ourMacStr[18];
    snprintf(ourMacStr, sizeof(ourMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             ourMac[0], ourMac[1], ourMac[2], ourMac[3], ourMac[4], ourMac[5]);
    
    Serial.printf("🎯 ESP-NOW RECEIVED from %s [%s]: %s\n", 
                  message->sender_id, message->message_type, message->payload);
    
    // Handle different message types
    String msgType = String(message->message_type);
    String payload = String(message->payload);
    
    if (msgType == "heartbeat") {
        Serial.printf("💓 Heartbeat from %s\n", message->sender_id);
        // Respond to heartbeat
        respondToPeerMessage(mac, "heartbeat_ack");
    } else if (msgType == "status") {
        Serial.printf("📊 Status update from %s: %s\n", message->sender_id, payload.c_str());
        // Respond with our status
        respondToPeerMessage(mac, "status_ack");
    } else if (msgType.endsWith("_ack")) {
        Serial.printf("✅ Acknowledgment from %s: %s\n", message->sender_id, msgType.c_str());
    } else {
        Serial.printf("❓ Unknown message type: %s\n", msgType.c_str());
    }
    
    // Report this ESP-NOW activity to MCP
    sendESPNowActivity(String(macStr), String(ourMacStr), 
                      String(message->message_type), String(message->payload));
}

void onESPNowSend(const uint8_t* mac_addr, esp_now_send_status_t status) {
    // Convert MAC to string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    // Get our MAC address
    uint8_t ourMac[6];
    esp_wifi_get_mac(WIFI_IF_STA, ourMac);
    char ourMacStr[18];
    snprintf(ourMacStr, sizeof(ourMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             ourMac[0], ourMac[1], ourMac[2], ourMac[3], ourMac[4], ourMac[5]);
    
    Serial.printf("ESP-NOW send to %s: %s\n", macStr, 
                  status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
    
    // Report this ESP-NOW activity to MCP
    String messageType = status == ESP_NOW_SEND_SUCCESS ? "send_success" : "send_failed";
    sendESPNowActivity(String(ourMacStr), String(macStr), messageType, "status_report");
}

void sendESPNowMessage(const uint8_t* peerMac, const String& messageType, const String& payload) {
    // Check if peer exists first
    if (!esp_now_is_peer_exist(peerMac)) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
        Serial.printf("ESP-NOW peer %s not found, attempting to add...\n", macStr);
        addESPNowPeer(peerMac);
        delay(10); // Small delay to ensure peer is added
        
        // Check again if peer was added successfully
        if (!esp_now_is_peer_exist(peerMac)) {
            Serial.printf("Failed to add ESP-NOW peer %s, cannot send message\n", macStr);
            return;
        }
    }
    
    esp_now_message_t message;
    memset(&message, 0, sizeof(message)); // Clear the structure
    strncpy(message.sender_id, botID.c_str(), sizeof(message.sender_id) - 1);
    strncpy(message.message_type, messageType.c_str(), sizeof(message.message_type) - 1);
    strncpy(message.payload, payload.c_str(), sizeof(message.payload) - 1);
    message.timestamp = millis();
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    Serial.printf("Sending ESP-NOW message to %s: type=%s, payload=%s\n", 
                  macStr, messageType.c_str(), payload.c_str());
    
    // Debug message structure
    Serial.printf("Message size: %d bytes\n", sizeof(message));
    Serial.printf("Message contents: sender_id='%s', message_type='%s', payload='%s', timestamp=%lu\n",
                  message.sender_id, message.message_type, message.payload, message.timestamp);
    
    esp_err_t result = esp_now_send(peerMac, (uint8_t*)&message, sizeof(message));
    
    Serial.printf("ESP-NOW send result: %d (0x%X) -> %s\n", result, result, esp_err_to_name(result));
    
    if (result == ESP_OK) {
        Serial.println("ESP-NOW message sent successfully");
    } else {
        Serial.printf("Error sending ESP-NOW message. Error code: %d (0x%X)\n", result, result);
        
        // Additional debugging info
        if (result == ESP_ERR_ESPNOW_NOT_INIT) {
            Serial.println("ESP-NOW not initialized");
        } else if (result == ESP_ERR_ESPNOW_ARG) {
            Serial.println("Invalid argument");
        } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
            Serial.println("Internal error");
        } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
            Serial.println("Out of memory");
        } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
            Serial.println("Peer not found");
        } else if (result == ESP_ERR_ESPNOW_IF) {
            Serial.println("Invalid interface");
        } else {
            Serial.printf("Unknown ESP-NOW error: %d (0x%X)\n", result, result);
        }
    }
}

void performBotTasks() {
    // Simulate battery drain
    static unsigned long lastBatteryUpdate = 0;
    if (millis() - lastBatteryUpdate > 60000) { // Update every minute
        botStatus.batteryLevel -= 0.1; // Drain 0.1% per minute
        if (botStatus.batteryLevel < 0) botStatus.batteryLevel = 0;
        lastBatteryUpdate = millis();
    }
    
    // Check for low battery
    if (botStatus.batteryLevel < 20 && botStatus.status != "low_battery") {
        botStatus.status = "low_battery";
        Serial.println("Warning: Low battery!");
    }
    
    // Simulate ESP-NOW communication periodically
    static unsigned long lastESPNowTest = 0;
    static unsigned long lastStatusShare = 0;
    
    if (millis() - lastESPNowTest > 30000) { // Every 30 seconds
        Serial.println("Sending ESP-NOW heartbeat to all peers...");
        sendHeartbeatToAllPeers();
        lastESPNowTest = millis();
    }
    
    // Send status updates to peers every 15 seconds
    if (millis() - lastStatusShare > 15000) { // Every 15 seconds
        Serial.println("Sharing status with all peers...");
        sendStatusToAllPeers();
        lastStatusShare = millis();
    }
    
    // Update WiFi signal strength
    if (WiFi.status() == WL_CONNECTED) {
        botStatus.wifiSignal = WiFi.RSSI();
    }
}

void checkForFirmwareUpdate() {
    if (WiFi.status() != WL_CONNECTED || mcpServerIP == "") {
        Serial.println("Cannot check for firmware update - WiFi not connected or MCP server not found");
        return;
    }

    Serial.println("Checking for new firmware...");

    HTTPClient http; // Create a new client for this request
    String url = "http://" + mcpServerIP + ":" + String(mcpServerPort) + mcpFirmwareEndpoint;
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        DynamicJsonDocument doc(512);
        deserializeJson(doc, payload);

        float latestVersion = doc["version"];
        const char* filename = doc["filename"];

        Serial.printf("Current firmware version: %.2f\n", FIRMWARE_VERSION);
        Serial.printf("Latest firmware version on server: %.2f\n", latestVersion);

        if (latestVersion > FIRMWARE_VERSION) {
            Serial.printf("New firmware available (v%.2f). Starting update from %s...\n", latestVersion, filename);
            
            String firmwareUrl = "http://" + mcpServerIP + ":" + String(mcpServerPort) + "/firmware/" + String(filename);
            
            WiFiClient client; // Create a client for the update

            // Use HTTPUpdate library
            t_httpUpdate_return ret = httpUpdate.update(client, firmwareUrl);

            switch (ret) {
                case HTTP_UPDATE_FAILED:
                    Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
                    break;
                case HTTP_UPDATE_NO_UPDATES:
                    Serial.println("HTTP_UPDATE_NO_UPDATES");
                    break;
                case HTTP_UPDATE_OK:
                    Serial.println("HTTP_UPDATE_OK"); // This will not be printed as the ESP32 will reboot
                    break;
            }
        } else {
            Serial.println("Firmware is up to date.");
        }
    }
    http.end();
}

// Function to add ESP-NOW peers (call this when you know other bot MAC addresses)
// Function to add ESP-NOW peers (call this when you know other bot MAC addresses)
void addESPNowPeer(const uint8_t* peerMac) {
    // Check if peer already exists
    bool peerExists = esp_now_is_peer_exist(peerMac);
    if (peerExists) {
        Serial.println("ESP-NOW peer already exists");
        return;
    }
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac, 6);
    
    // Get current WiFi channel and use it for ESP-NOW
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&peerInfo.channel, &secondChan);
    
    peerInfo.encrypt = false;
    
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result != ESP_OK) {
        Serial.printf("Failed to add ESP-NOW peer. Error code: %d\n", result);
        // Try with channel 0 as fallback
        peerInfo.channel = 0;
        result = esp_now_add_peer(&peerInfo);
        if (result != ESP_OK) {
            Serial.printf("Failed to add ESP-NOW peer even with channel 0. Error code: %d\n", result);
            return;
        }
    }
    
    Serial.printf("ESP-NOW peer added successfully on channel %d\n", peerInfo.channel);
}

// Example function to send heartbeat to all peers
void sendHeartbeatToAllPeers() {
    Serial.println("Sending heartbeat to known peers...");
    
    // Send to all known peers from our array
    for (int i = 0; i < sizeof(knownPeers) / sizeof(knownPeers[0]); i++) {
        // Skip broadcast address and zero MAC addresses
        if (knownPeers[i][0] == 0xFF || 
            (knownPeers[i][0] == 0x00 && knownPeers[i][1] == 0x00 && 
             knownPeers[i][2] == 0x00 && knownPeers[i][3] == 0x00 && 
             knownPeers[i][4] == 0x00 && knownPeers[i][5] == 0x00)) {
            continue;
        }
        
        Serial.printf("Sending heartbeat to peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      knownPeers[i][0], knownPeers[i][1], knownPeers[i][2],
                      knownPeers[i][3], knownPeers[i][4], knownPeers[i][5]);
        
        sendESPNowMessage(knownPeers[i], "heartbeat", botID);
    }
}

// Function to send status updates to all peers
void sendStatusToAllPeers() {
    // Create status message with current bot information
    DynamicJsonDocument statusDoc(128);
    statusDoc["battery"] = botStatus.batteryLevel;
    statusDoc["wifi_signal"] = botStatus.wifiSignal;
    statusDoc["uptime"] = millis() / 1000;
    statusDoc["status"] = botStatus.status;
    
    String statusPayload;
    serializeJson(statusDoc, statusPayload);
    
    Serial.println("Sending status to known peers...");
    
    // Send to all known peers from our array
    for (int i = 0; i < sizeof(knownPeers) / sizeof(knownPeers[0]); i++) {
        // Skip broadcast address and zero MAC addresses
        if (knownPeers[i][0] == 0xFF || 
            (knownPeers[i][0] == 0x00 && knownPeers[i][1] == 0x00 && 
             knownPeers[i][2] == 0x00 && knownPeers[i][3] == 0x00 && 
             knownPeers[i][4] == 0x00 && knownPeers[i][5] == 0x00)) {
            continue;
        }
        
        Serial.printf("📊 Sending status to peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      knownPeers[i][0], knownPeers[i][1], knownPeers[i][2],
                      knownPeers[i][3], knownPeers[i][4], knownPeers[i][5]);
        
        sendESPNowMessage(knownPeers[i], "status", statusPayload);
    }
}

// Function to respond to peer messages
void respondToPeerMessage(const uint8_t* peerMac, const String& responseType) {
    String responsePayload = botID + "_response";
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    
    Serial.printf("📤 Responding to %s with: %s\n", macStr, responseType.c_str());
    sendESPNowMessage(peerMac, responseType, responsePayload);
}