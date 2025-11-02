/*
 * ESP-NOW Communication Example
 * Demonstrates bot-to-bot communication using ESP-NOW protocol
 * Part of the Master Control Program IoT Bot Network
 */

#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Bot Configuration
String botID = "ESP32_Bot_" + String(ESP.getEfuseMac(), HEX);
bool isMaster = false; // Set to true for one bot to act as coordinator

// ESP-NOW Message Structure
typedef struct {
    char sender_id[32];
    char message_type[16];
    char payload[200];
    uint32_t timestamp;
    uint8_t hop_count;
} esp_now_message_t;

// Forward declarations
void addKnownPeers();
void sendHeartbeat();
void broadcastSensorData();
void broadcastMessage(esp_now_message_t* message);
void sendDirectMessage(const uint8_t* targetMac, const String& messageType, const String& payload);
void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status);
void onDataReceive(const uint8_t* mac, const uint8_t* incomingData, int len);
void handleIncomingMessage(const uint8_t* senderMac, esp_now_message_t* message);
void handleHeartbeat(const uint8_t* senderMac, esp_now_message_t* message);
void handleSensorData(const uint8_t* senderMac, esp_now_message_t* message);
void handleCommand(const uint8_t* senderMac, esp_now_message_t* message);
void handleResponse(const uint8_t* senderMac, esp_now_message_t* message);
void handleEmergency(const uint8_t* senderMac, esp_now_message_t* message);
String executeCommand(const String& command);
void performNetworkScan();
void handleSerialCommands();
bool parseMacAddress(const String& macStr, uint8_t* mac);
void printBotInfo();
void printPeerInfo();
void printNetworkStats();
void printHelp();

// Known peer MAC addresses (add your bot MAC addresses here)
uint8_t knownPeers[][6] = {
    {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC}, // Example MAC 1
    {0x24, 0x6F, 0x28, 0xDD, 0xEE, 0xFF}, // Example MAC 2
    // Add more MAC addresses as needed
};
const int numKnownPeers = sizeof(knownPeers) / sizeof(knownPeers[0]);

// Message handling
struct {
    unsigned long lastHeartbeat;
    unsigned long lastDataBroadcast;
    unsigned long lastNetworkScan;
    int messagesSent;
    int messagesReceived;
} networkStats;

void setup() {
    Serial.begin(115200);
    Serial.println("ESP-NOW Bot Network Node Starting...");
    
    // Initialize WiFi in station mode
    WiFi.mode(WIFI_STA);
    Serial.print("Bot MAC Address: ");
    Serial.println(WiFi.macAddress());
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    
    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceive);
    
    // Add known peers
    addKnownPeers();
    
    // Initialize network stats
    networkStats.lastHeartbeat = 0;
    networkStats.lastDataBroadcast = 0;
    networkStats.lastNetworkScan = 0;
    networkStats.messagesSent = 0;
    networkStats.messagesReceived = 0;
    
    Serial.println("ESP-NOW Bot Node initialized");
    printBotInfo();
}

void loop() {
    unsigned long currentTime = millis();
    
    // Send heartbeat every 30 seconds
    if (currentTime - networkStats.lastHeartbeat > 30000) {
        sendHeartbeat();
        networkStats.lastHeartbeat = currentTime;
    }
    
    // Send sensor data every 60 seconds
    if (currentTime - networkStats.lastDataBroadcast > 60000) {
        broadcastSensorData();
        networkStats.lastDataBroadcast = currentTime;
    }
    
    // Perform network scan every 5 minutes (if master)
    if (isMaster && currentTime - networkStats.lastNetworkScan > 300000) {
        performNetworkScan();
        networkStats.lastNetworkScan = currentTime;
    }
    
    // Handle serial commands
    handleSerialCommands();
    
    delay(1000);
}

void addKnownPeers() {
    for (int i = 0; i < numKnownPeers; i++) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, knownPeers[i], 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        
        if (esp_now_add_peer(&peerInfo) == ESP_OK) {
            Serial.printf("Added peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                         knownPeers[i][0], knownPeers[i][1], knownPeers[i][2],
                         knownPeers[i][3], knownPeers[i][4], knownPeers[i][5]);
        }
    }
}

void sendHeartbeat() {
    esp_now_message_t message;
    strcpy(message.sender_id, botID.c_str());
    strcpy(message.message_type, "heartbeat");
    
    // Create heartbeat payload
    DynamicJsonDocument doc(200);
    doc["uptime"] = millis() / 1000;
    doc["battery"] = random(500, 1000) / 10.0; // Simulate battery level
    doc["status"] = "active";
    doc["msg_sent"] = networkStats.messagesSent;
    doc["msg_recv"] = networkStats.messagesReceived;
    
    String payload;
    serializeJson(doc, payload);
    strcpy(message.payload, payload.c_str());
    message.timestamp = millis();
    message.hop_count = 0;
    
    // Broadcast to all peers
    broadcastMessage(&message);
    
    Serial.println("Heartbeat sent to network");
}

void broadcastSensorData() {
    esp_now_message_t message;
    strcpy(message.sender_id, botID.c_str());
    strcpy(message.message_type, "sensor_data");
    
    // Create sensor data payload
    DynamicJsonDocument doc(200);
    doc["temperature"] = random(200, 350) / 10.0;
    doc["humidity"] = random(300, 800) / 10.0;
    doc["light"] = random(0, 1023);
    doc["motion"] = random(0, 2) == 1;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    strcpy(message.payload, payload.c_str());
    message.timestamp = millis();
    message.hop_count = 0;
    
    // Broadcast to all peers
    broadcastMessage(&message);
    
    Serial.println("Sensor data broadcast to network");
}

void broadcastMessage(esp_now_message_t* message) {
    esp_now_peer_info_t peer;
    esp_now_peer_num_t peerNum;
    
    esp_now_get_peer_num(&peerNum);
    
    for (int i = 0; i < peerNum.total_num; i++) {
        if (esp_now_fetch_peer(true, &peer) == ESP_OK) {
            esp_err_t result = esp_now_send(peer.peer_addr, (uint8_t*)message, sizeof(*message));
            if (result == ESP_OK) {
                networkStats.messagesSent++;
            }
        }
    }
}

void sendDirectMessage(const uint8_t* targetMac, const String& messageType, const String& payload) {
    esp_now_message_t message;
    strcpy(message.sender_id, botID.c_str());
    strcpy(message.message_type, messageType.c_str());
    strcpy(message.payload, payload.c_str());
    message.timestamp = millis();
    message.hop_count = 0;
    
    esp_err_t result = esp_now_send(targetMac, (uint8_t*)&message, sizeof(message));
    if (result == ESP_OK) {
        networkStats.messagesSent++;
        Serial.printf("Direct message sent to %02X:%02X:%02X:%02X:%02X:%02X\n",
                     targetMac[0], targetMac[1], targetMac[2], targetMac[3], targetMac[4], targetMac[5]);
    }
}

void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    Serial.printf("Send to %s: %s\n", macStr, status == ESP_NOW_SEND_SUCCESS ? "Success" : "Failed");
}

void onDataReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    esp_now_message_t* message = (esp_now_message_t*)incomingData;
    networkStats.messagesReceived++;
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    Serial.printf("Received from %s (%s): %s -> %s\n", 
                  macStr, message->sender_id, message->message_type, message->payload);
    
    // Handle different message types
    handleIncomingMessage(mac, message);
}

void handleIncomingMessage(const uint8_t* senderMac, esp_now_message_t* message) {
    String messageType = String(message->message_type);
    String payload = String(message->payload);
    
    if (messageType == "heartbeat") {
        handleHeartbeat(senderMac, message);
    } else if (messageType == "sensor_data") {
        handleSensorData(senderMac, message);
    } else if (messageType == "command") {
        handleCommand(senderMac, message);
    } else if (messageType == "response") {
        handleResponse(senderMac, message);
    } else if (messageType == "emergency") {
        handleEmergency(senderMac, message);
    } else {
        Serial.println("Unknown message type: " + messageType);
    }
}

void handleHeartbeat(const uint8_t* senderMac, esp_now_message_t* message) {
    // Parse heartbeat data
    DynamicJsonDocument doc(200);
    deserializeJson(doc, message->payload);
    
    Serial.printf("Heartbeat from %s: Uptime=%ds, Battery=%.1f%%\n", 
                  message->sender_id, 
                  doc["uptime"].as<int>(),
                  doc["battery"].as<float>());
}

void handleSensorData(const uint8_t* senderMac, esp_now_message_t* message) {
    // Parse sensor data
    DynamicJsonDocument doc(200);
    deserializeJson(doc, message->payload);
    
    Serial.printf("Sensor data from %s: Temp=%.1fC, Humidity=%.1f%%, Light=%d\n",
                  message->sender_id,
                  doc["temperature"].as<float>(),
                  doc["humidity"].as<float>(),
                  doc["light"].as<int>());
}

void handleCommand(const uint8_t* senderMac, esp_now_message_t* message) {
    // Parse command
    DynamicJsonDocument doc(200);
    deserializeJson(doc, message->payload);
    
    String command = doc["cmd"].as<String>();
    Serial.println("Received command: " + command);
    
    // Execute command and send response
    String response = executeCommand(command);
    sendDirectMessage(senderMac, "response", response);
}

void handleResponse(const uint8_t* senderMac, esp_now_message_t* message) {
    Serial.printf("Response from %s: %s\n", message->sender_id, message->payload);
}

void handleEmergency(const uint8_t* senderMac, esp_now_message_t* message) {
    Serial.printf("EMERGENCY from %s: %s\n", message->sender_id, message->payload);
    
    // Relay emergency message to other peers if hop count is low
    if (message->hop_count < 3) {
        message->hop_count++;
        broadcastMessage(message);
    }
}

String executeCommand(const String& command) {
    if (command == "status") {
        DynamicJsonDocument doc(200);
        doc["bot_id"] = botID;
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();
        doc["msgs_sent"] = networkStats.messagesSent;
        doc["msgs_recv"] = networkStats.messagesReceived;
        
        String response;
        serializeJson(doc, response);
        return response;
    } else if (command == "ping") {
        return "pong";
    } else if (command == "restart") {
        ESP.restart();
        return "restarting";
    } else {
        return "unknown_command";
    }
}

void performNetworkScan() {
    Serial.println("Performing network scan (Master node)...");
    
    // Send ping to all known peers
    esp_now_message_t message;
    strcpy(message.sender_id, botID.c_str());
    strcpy(message.message_type, "command");
    strcpy(message.payload, "{\"cmd\":\"ping\"}");
    message.timestamp = millis();
    message.hop_count = 0;
    
    broadcastMessage(&message);
    
    Serial.println("Network scan initiated");
}

void handleSerialCommands() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "status") {
            printBotInfo();
        } else if (command == "peers") {
            printPeerInfo();
        } else if (command == "stats") {
            printNetworkStats();
        } else if (command == "heartbeat") {
            sendHeartbeat();
        } else if (command == "sensor") {
            broadcastSensorData();
        } else if (command.startsWith("send ")) {
            // Format: send <mac> <message>
            int firstSpace = command.indexOf(' ', 5);
            if (firstSpace > 0) {
                String macStr = command.substring(5, firstSpace);
                String payload = command.substring(firstSpace + 1);
                
                // Parse MAC address (format: AA:BB:CC:DD:EE:FF)
                uint8_t targetMac[6];
                if (parseMacAddress(macStr, targetMac)) {
                    sendDirectMessage(targetMac, "command", payload);
                } else {
                    Serial.println("Invalid MAC address format");
                }
            }
        } else if (command == "help") {
            printHelp();
        } else {
            Serial.println("Unknown command: " + command);
        }
    }
}

bool parseMacAddress(const String& macStr, uint8_t* mac) {
    if (macStr.length() != 17) return false;
    
    for (int i = 0; i < 6; i++) {
        String byteStr = macStr.substring(i * 3, i * 3 + 2);
        mac[i] = strtol(byteStr.c_str(), NULL, 16);
    }
    return true;
}

void printBotInfo() {
    Serial.println("\n=== Bot Information ===");
    Serial.println("Bot ID: " + botID);
    Serial.println("MAC Address: " + WiFi.macAddress());
    Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("Is Master: " + String(isMaster ? "Yes" : "No"));
    Serial.println("========================\n");
}

void printPeerInfo() {
    Serial.println("\n=== Peer Information ===");
    esp_now_peer_num_t peerNum;
    esp_now_get_peer_num(&peerNum);
    Serial.println("Total peers: " + String(peerNum.total_num));
    
    esp_now_peer_info_t peer;
    for (int i = 0; i < peerNum.total_num; i++) {
        if (esp_now_fetch_peer(true, &peer) == ESP_OK) {
            Serial.printf("Peer %d: %02X:%02X:%02X:%02X:%02X:%02X\n", i,
                         peer.peer_addr[0], peer.peer_addr[1], peer.peer_addr[2],
                         peer.peer_addr[3], peer.peer_addr[4], peer.peer_addr[5]);
        }
    }
    Serial.println("=========================\n");
}

void printNetworkStats() {
    Serial.println("\n=== Network Statistics ===");
    Serial.println("Messages Sent: " + String(networkStats.messagesSent));
    Serial.println("Messages Received: " + String(networkStats.messagesReceived));
    Serial.println("Last Heartbeat: " + String((millis() - networkStats.lastHeartbeat) / 1000) + "s ago");
    Serial.println("Last Data Broadcast: " + String((millis() - networkStats.lastDataBroadcast) / 1000) + "s ago");
    Serial.println("===========================\n");
}

void printHelp() {
    Serial.println("\n=== Available Commands ===");
    Serial.println("status    - Show bot information");
    Serial.println("peers     - Show peer information");
    Serial.println("stats     - Show network statistics");
    Serial.println("heartbeat - Send heartbeat message");
    Serial.println("sensor    - Broadcast sensor data");
    Serial.println("send <mac> <message> - Send direct message");
    Serial.println("help      - Show this help");
    Serial.println("===========================\n");
}