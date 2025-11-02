# Getting Started with MCP

## Quick Start Guide

### 1. Install Python Dependencies

```bash
pip install -r requirements.txt
```

### 2. Configure the System

Edit `config/config.json` with your network settings:

```json
{
  "server": {
    "host": "0.0.0.0", // Listens on all available network interfaces
    "port": 8080       // The port the server will run on
  }
}
```

### 3. Start the MCP Server

```bash
python mcp_server.py
```

### 4. Access the Dashboard

Open your web browser and navigate to:

- <http://localhost:8080>

### 5. Set Up ESP32 Bots

1. Install Arduino IDE and ESP32 board package
2. Configure WiFi credentials in the ESP32 examples
3. Upload `bot_wifi_client.ino` to your ESP32 devices
4. Monitor bot connections in the dashboard

## System Architecture

The Master Control Program consists of several key components:

- **MCP Server** (`mcp_server.py`) - Main application with REST API and WebSocket support
- **Bot Manager** (`bot_manager.py`) - Handles bot registration and status tracking
- **Database Manager** (`database.py`) - SQLite database operations for data persistence
- **Web Dashboard** (`web_dashboard/`) - Real-time visualization interface
- **ESP32 Examples** (`esp32_examples/`) - Arduino sketches for bot communication

## Network Communication

### Bot to MCP (WiFi)

Bots connect to your WiFi network and send HTTP POST requests to the MCP server:

- **Endpoint**: `POST /api/bot/status`
- **Format**: JSON
- **Frequency**: Every 10 seconds (configurable)

Example bot status message:

```json
{
  "bot_id": "ESP32_Bot_ABC123",
  "status": "active",
  "battery_level": 85.5,
  "wifi_signal": -45,
  "uptime_seconds": 3600,
  "sensor_data": {
    "temperature": 23.5,
    "humidity": 65.2
  }
}
```

### Bot to Bot (ESP-NOW)

Bots use ESP-NOW for direct communication without requiring internet connectivity:

- **Protocol**: ESP-NOW (IEEE 802.11 vendor-specific)
- **Range**: Up to 220 meters (line of sight)
- **Message Types**: heartbeat, sensor_data, command, response, emergency

## Dashboard Features

The web dashboard provides real-time monitoring with:

- **Bot Status Overview** - Live status of all registered bots
- **Network Topology** - Visual representation of bot network
- **ESP-NOW Activity** - Monitor inter-bot communications
- **Analytics Charts** - Battery levels, signal strength, activity trends
- **Bot Details** - Detailed information and history for each bot

## Configuration Options

### Server Configuration

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080,
    "debug": false
  },
  "monitoring": {
    "bot_timeout_seconds": 30,
    "health_check_interval_seconds": 10
  }
}
```

### Bot Configuration

ESP32 bots can be configured by modifying the Arduino sketch:

```cpp
// WiFi Settings
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MCP Server Settings
const char* mcpServerIP = "192.168.1.100";
const int mcpServerPort = 8080;

// Update intervals
const unsigned long statusUpdateInterval = 10000; // 10 seconds
```

## Advanced Features

### Data Storage

All bot data is automatically stored in SQLite database:

- Bot registration and status history
- ESP-NOW message logs
- System events and statistics
- Automatic data cleanup (configurable retention period)

### Real-time Updates

The dashboard uses WebSocket connections for real-time updates:

- Live bot status changes
- ESP-NOW activity monitoring
- Network topology updates
- System health metrics

### Silent Monitoring

The MCP operates as a passive observer:

- No interference with bot operations
- Minimal network overhead
- Comprehensive logging for analysis
- Non-intrusive health monitoring

## Troubleshooting

### Common Issues

#### Q: Bots not connecting to MCP server

- Check WiFi credentials in ESP32 code
- Verify MCP server IP address is correct
- Ensure firewall allows traffic on port 8080
- Check that MCP server is running

#### Q: Dashboard shows no data

- Verify WebSocket connection (check browser console)
- Ensure at least one bot is connected and sending data
- Check MCP server logs for errors

#### Q: ESP-NOW communication not working

- Verify MAC addresses are correctly configured
- Ensure all devices are on the same WiFi channel
- Check that ESP-NOW is properly initialized

### Logging

View MCP server logs:

- Console output shows real-time activity
- Log files are stored in `data/logs/mcp.log`
- Adjust log level in `config/config.json`

## API Reference

### REST Endpoints

- `GET /` - Dashboard interface
- `POST /api/bot/status` - Receive bot status update
- `POST /api/esp-now/message` - Receive ESP-NOW activity report
- `GET /api/bots` - Get all registered bots
- `GET /api/bots/{bot_id}` - Get specific bot details
- `GET /api/esp-now/activity` - Get ESP-NOW activity history

### WebSocket Events

- `initial_data` - Initial bot data on connection
- `bot_update` - Real-time bot status changes
- `esp_now_activity` - ESP-NOW message activity
- `heartbeat` - Connection keep-alive

## Security Considerations

For production deployments:

1. **Network Security**
   - Use WPA2/WPA3 for WiFi networks
   - Consider VPN for remote access
   - Implement network segmentation

2. **Application Security**
   - Enable API key authentication in config
   - Use HTTPS for web dashboard
   - Implement rate limiting

3. **Device Security**
   - Secure boot on ESP32 devices
   - Encrypt ESP-NOW communications
   - Regular firmware updates

## Performance Tips

- **Database Optimization**
  - Regular cleanup of old data
  - Index optimization for large datasets
  - Consider PostgreSQL for high-volume deployments

- **Network Optimization**
  - Adjust bot update intervals based on needs
  - Use message compression for large payloads
  - Monitor bandwidth usage

- **Scalability**
  - Current design supports 100+ bots
  - Use load balancing for larger deployments
  - Consider microservices architecture for enterprise use

## License

This project is released under the MIT License. See LICENSE file for details.
