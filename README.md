# Master Control Program (MCP) - IoT Bot Monitoring System

A Python-based Master Control Program designed to silently monitor and collect data from multiple ESP-based bots that communicate via ESP-NOW and transmit status information via WiFi.

## System Overview

The MCP acts as a central monitoring hub that:

- Receives data from ESP-based bots via WiFi (HTTP/WebSocket)
- Monitors bot-to-bot ESP-NOW communications
- Provides real-time visualization dashboard
- Maintains comprehensive logging and data storage
- Operates as a silent observer without interfering with bot operations

## Features

- **Silent Monitoring**: Non-intrusive observation of bot network
- **Real-time Dashboard**: Web-based visualization of bot status and communications
- **Data Logging**: Comprehensive storage of all bot communications and status
- **Health Monitoring**: Track bot connectivity and operational status
- **ESP-NOW Tracking**: Monitor inter-bot communications
- **Scalable Architecture**: Support for multiple bots with async processing

## Quick Start

1. **Install Dependencies**:

   ```bash
   pip install -r requirements.txt
   ```

2. **Configure Settings**:

   ```bash
   cp config/config.example.json config/config.json
   # Edit config.json with your network settings
   ```

3. **Start the MCP Server**:

   ```bash
   python mcp_server.py
   ```

4. **Access Dashboard**:
   Open `http://localhost:8080` in your browser

## Project Structure

See detailed structure in [`docs/PROJECT_STRUCTURE.md`](docs/PROJECT_STRUCTURE.md)

## 🤖 Quick Bot Creation

Create new ESP32 bots instantly with the generic template:

```bash
# Create a new bot (from project root)
.\create_bot.bat "My_Sensor_Bot"

# Navigate to your new bot
cd esp32_examples\bots\My_Sensor_Bot

# Customize and build
pio run
pio run --target upload
```

## Documentation

- **[📋 Project Structure](docs/PROJECT_STRUCTURE.md)** - Complete project organization
- **[🤖 Generic Bot Guide](docs/GENERIC_BOT_GUIDE.md)** - Bot creation and customization  
- **[🎯 Template System Summary](docs/TEMPLATE_SYSTEM_SUMMARY.md)** - Template features overview
- **[🔧 Scripts](scripts/README.md)** - Automation script documentation
master-control-program/
├── mcp_server.py           # Main server application
├── bot_manager.py          # Bot management and data handling
├── database.py             # Database operations
├── config/
│   ├── config.json         # Main configuration
│   └── logging_config.json # Logging configuration
├── web_dashboard/
│   ├── index.html          # Main dashboard
│   ├── static/
│   │   ├── css/
│   │   ├── js/
│   │   └── assets/
├── esp32_examples/
│   ├── bot_wifi_client/    # ESP32 WiFi client code
│   └── esp_now_example/    # ESP-NOW communication example
├── data/
│   ├── mcp.db             # SQLite database
│   └── logs/              # Log files
└── requirements.txt       # Python dependencies

```txt

## Communication Protocol

### Bot to MCP (WiFi)

- **Endpoint**: `POST /api/bot/status`
- **Format**: JSON
- **Content**: Bot ID, status, sensor data, ESP-NOW activity

### ESP-NOW Monitoring

- Bots report ESP-NOW message activity to MCP
- Includes sender/receiver information and message types
- Enables tracking of bot-to-bot communication patterns

## Configuration

Edit `config/config.json`:

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 8080
  },
  "database": {
    "path": "data/mcp.db"
  },
  "monitoring": {
    "bot_timeout": 30,
    "health_check_interval": 10
  }
}
```

## ESP32 Bot Integration

1. Use provided example code in `esp32_examples/`
2. Configure WiFi credentials for MCP connection
3. Implement ESP-NOW for bot-to-bot communication
4. Send periodic status updates to MCP

## Development

- Python 3.8+ required
- Uses asyncio for concurrent bot handling
- FastAPI for REST API and WebSocket support
- SQLite for lightweight data storage
- Real-time updates via WebSocket connections

## Monitoring Dashboard

The web dashboard provides:

- Live bot status and connectivity
- ESP-NOW communication visualization
- Historical data charts
- Bot network topology
- System health metrics

## License

MIT License - see LICENSE file for details
