# Master Control Program - IoT Bot Monitoring System

This project implements a Python-based Master Control Program (MCP) for monitoring multiple ESP-based bots that communicate via ESP-NOW and transmit data to the MCP via WiFi.

## Project Structure
- `mcp_server.py` - Main server application with WiFi communication handling
- `bot_manager.py` - Bot registration, health monitoring, and data management
- `web_dashboard/` - Real-time web dashboard for data visualization
- `config/` - Configuration files and settings
- `esp32_examples/` - Example ESP32 code for bot communication
- `data/` - SQLite database and log files

## Development Guidelines
- Focus on silent observation and minimal interference with bot operations
- Implement robust error handling for network communications
- Use asynchronous programming for handling multiple bot connections
- Maintain comprehensive logging for debugging and monitoring
- Ensure real-time data processing and visualization

## Communication Protocol
- Bots use ESP-NOW for inter-bot communication
- Bots transmit status/data to MCP via HTTP/WebSocket over WiFi
- MCP acts as silent observer, logging all communications
- JSON message format for structured data exchange

## Technology Stack
- Python 3.8+ with asyncio for async operations
- FastAPI/WebSocket for WiFi communication server
- SQLite for data storage
- HTML/CSS/JavaScript for web dashboard
- Real-time data visualization with Chart.js or similar