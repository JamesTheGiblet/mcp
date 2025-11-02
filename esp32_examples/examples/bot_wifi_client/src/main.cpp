/*
 * ESP32 Bot WiFi Client
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
#include "config.h"

// Forward declarations
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

// ESP-NOW Configuration
// Add the MAC addresses of your other bots here.
// MAC addresses must be in uint8_t format.
// Example: {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F}
uint8_t knownPeers[][6] = {
    // For Bot 1, put Bot 2's MAC here. For Bot 2, put Bot 1's MAC here.
    // {0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5} 
};

typedef struct {
    char sender_id[32];
    char message_type[16];
    char payload[200];
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

    // Add a broadcast peer for ESP-NOW heartbeats
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    addESPNowPeer(broadcastAddress);

    // Add specific known peers
    for (int i = 0; i < sizeof(knownPeers) / sizeof(knownPeers[0]); i++) {
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
            mcpServerPort = 8080; // Default server port is 8080
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
    
    Serial.printf("ESP-NOW received from %s: %s\n", macStr, message->payload);
    
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
    esp_now_message_t message;
    strncpy(message.sender_id, botID.c_str(), sizeof(message.sender_id) - 1);
    strncpy(message.message_type, messageType.c_str(), sizeof(message.message_type) - 1);
    strncpy(message.payload, payload.c_str(), sizeof(message.payload) - 1);
    message.timestamp = millis();
    
    esp_err_t result = esp_now_send(peerMac, (uint8_t*)&message, sizeof(message));
    
    if (result == ESP_OK) {
        Serial.println("ESP-NOW message sent successfully");
    } else {
        Serial.println("Error sending ESP-NOW message");
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
    if (millis() - lastESPNowTest > 30000) { // Every 30 seconds
        // This would normally send to known peer MAC addresses
        Serial.println("Sending ESP-NOW heartbeat to all peers...");
        sendHeartbeatToAllPeers();
        lastESPNowTest = millis();
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
void addESPNowPeer(const uint8_t* peerMac) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerMac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer");
        return;
    }
    
    Serial.println("ESP-NOW peer added successfully");
}

// Example function to send heartbeat to all peers
void sendHeartbeatToAllPeers() {
    esp_now_peer_info_t peer;
    esp_now_peer_num_t peerNum;
    
    esp_now_get_peer_num(&peerNum);
    
    for (int i = 0; i < peerNum.total_num; i++) {
        if (esp_now_fetch_peer(true, &peer) == ESP_OK) {
            sendESPNowMessage(peer.peer_addr, "heartbeat", botID);
        }
    }
}