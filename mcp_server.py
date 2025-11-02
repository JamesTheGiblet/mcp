"""
Master Control Program (MCP) Server
Monitors ESP-based bots via WiFi communication and provides real-time dashboard
"""

import asyncio
import json
import logging
import os
from datetime import datetime, timedelta, timezone
from socket import inet_aton
from pathlib import Path
from typing import Dict, List, Optional

import uvicorn
from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect, Security, Depends, UploadFile, File, Form
from fastapi.security import APIKeyHeader
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse, FileResponse
import socket
from zeroconf import ServiceInfo, Zeroconf
from pydantic import BaseModel, Field

from bot_manager import BotManager
from database import DatabaseManager


# Data models for API
class BotStatusData(BaseModel):
    bot_id: str = Field(..., description="Unique identifier for the bot")
    timestamp: Optional[datetime] = Field(default_factory=lambda: datetime.now(timezone.utc))
    status: str = Field(..., description="Bot operational status")
    battery_level: Optional[float] = Field(None, ge=0, le=100)
    wifi_signal: Optional[int] = Field(None, ge=-100, le=0)
    mac_address: Optional[str] = None
    location: Optional[Dict[str, float]] = None
    sensor_data: Optional[Dict] = None
    esp_now_activity: Optional[List[Dict]] = None
    uptime_seconds: Optional[int] = Field(None, ge=0)


class ESPNowMessage(BaseModel):
    sender_mac: str
    receiver_mac: str
    message_type: str
    payload: Dict
    timestamp: Optional[datetime] = Field(default_factory=lambda: datetime.now(timezone.utc))
    rssi: Optional[int] = None


class MCPServer:
    def __init__(self, config_path: str = "config/config.json"):
        self.config = self._load_config(config_path)
        self.app = FastAPI(title="Master Control Program", version="1.0.0")
        self.bot_manager = BotManager(self.config)
        self.db_manager = DatabaseManager(self.config["database"]["path"])
        self.websocket_connections: List[WebSocket] = []
        self.zeroconf = None
        self.firmware_dir = Path("data/firmware")
        self.firmware_version_file = self.firmware_dir / "version.json"
        self.service_info = None
        
        self._setup_logging()
        self._setup_middleware()
        self._setup_routes()
        
        self.logger = logging.getLogger(__name__)
        self.logger.info("MCP Server initialized")

    def _load_config(self, config_path: str) -> Dict:
        """Load configuration from JSON file"""
        try:
            with open(config_path, 'r') as f:
                return json.load(f)
        except FileNotFoundError:
            self.logger.error(f"Config file not found: {config_path}")
            return self._default_config()
        except json.JSONDecodeError as e:
            self.logger.error(f"Invalid JSON in config file: {e}")
            return self._default_config()

    def _default_config(self) -> Dict:
        """Return default configuration"""
        return {
            "server": {"host": "0.0.0.0", "port": 8080, "debug": False},
            "database": {"path": "data/mcp.db"},
            "monitoring": {"bot_timeout_seconds": 30, "health_check_interval_seconds": 10, "data_retention_days": 30},
            "security": {"enable_cors": True, "allowed_origins": ["*"], "api_key": None},
            "logging": {"level": "INFO", "file_enabled": True, "console_enabled": True}
        }

    def _setup_logging(self):
        """Setup logging configuration"""
        log_config = self.config.get("logging", {})
        level = getattr(logging, log_config.get("level", "INFO"))
        
        # Create formatter
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        # Setup root logger
        logger = logging.getLogger()
        logger.setLevel(level)
        
        # Console handler
        if log_config.get("console_enabled", True):
            console_handler = logging.StreamHandler()
            console_handler.setFormatter(formatter)
            logger.addHandler(console_handler)
        
        # File handler
        if log_config.get("file_enabled", True):
            os.makedirs("data/logs", exist_ok=True)
            file_handler = logging.FileHandler("data/logs/mcp.log")
            file_handler.setFormatter(formatter)
            logger.addHandler(file_handler)

    def _setup_middleware(self):
        """Setup FastAPI middleware"""
        if self.config.get("security", {}).get("enable_cors", True):
            self.app.add_middleware(
                CORSMiddleware,
                allow_origins=["*"],  # Allow all origins for local network access
                allow_credentials=True,
                allow_methods=["*"],
                allow_headers=["*"],
            )

        # Add startup event handler
        @self.app.on_event("startup")
        async def startup_event():
            await self.start_background_tasks()

        @self.app.on_event("shutdown")
        def shutdown_event():
            self.logger.info("Shutting down mDNS service.")
            self._unregister_mdns_service()

    def _setup_routes(self):
        """Setup API routes"""

        # API Key Security Dependency
        api_key_header = APIKeyHeader(name="X-API-Key", auto_error=False)

        async def verify_api_key(api_key: str = Security(api_key_header)):
            """Verify the provided API key against the server configuration."""
            expected_api_key = self.config.get("security", {}).get("api_key")
            api_key_required = self.config.get("security", {}).get("api_key_required", False)
            
            # If API key is not required, allow the request.
            if not api_key_required:
                return

            if not api_key or api_key != expected_api_key:
                self.logger.warning(f"Unauthorized API access attempt with key: {api_key}")
                raise HTTPException(status_code=401, detail="Invalid or missing API Key")
        
        # Mount static files for dashboard
        if os.path.exists("web_dashboard"):
            self.app.mount("/static", StaticFiles(directory="web_dashboard/static"), name="static")
        
        # Mount firmware directory for OTA downloads
        self.firmware_dir.mkdir(exist_ok=True)
        self.app.mount("/firmware", StaticFiles(directory=self.firmware_dir), name="firmware")


        @self.app.get("/", response_class=HTMLResponse)
        async def dashboard():
            """Serve the main dashboard"""
            dashboard_path = Path("web_dashboard/index.html")
            if dashboard_path.exists():
                return FileResponse(dashboard_path)
            else:
                return HTMLResponse(
                    "<h1>MCP Dashboard</h1><p>Dashboard files not found. "
                    "Please ensure web_dashboard/index.html exists.</p>"
                )

        @self.app.post("/api/bot/status", dependencies=[Depends(verify_api_key)])
        async def receive_bot_status(data: BotStatusData):
            """Receive status update from a bot"""
            try:
                self.logger.info(f"Received status from bot {data.bot_id}")
                
                # Update bot in manager
                await self.bot_manager.update_bot_status(data)
                
                # Store in database
                await self.db_manager.store_bot_status(data)
                
                # Broadcast to connected WebSocket clients
                await self._broadcast_bot_update(data)
                
                return {"status": "success", "message": "Status received"}
                
            except Exception as e:
                self.logger.error(f"Error processing bot status: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.post("/api/esp-now/message", dependencies=[Depends(verify_api_key)])
        async def receive_esp_now_message(data: ESPNowMessage):
            """Receive ESP-NOW message report from a bot"""
            try:
                self.logger.debug(f"ESP-NOW: {data.sender_mac} -> {data.receiver_mac}")
                
                # Store ESP-NOW activity
                await self.db_manager.store_esp_now_message(data)
                
                # Broadcast to dashboard
                await self._broadcast_esp_now_activity(data)
                
                return {"status": "success", "message": "ESP-NOW message logged"}
                
            except Exception as e:
                self.logger.error(f"Error processing ESP-NOW message: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.get("/api/bots")
        async def get_all_bots():
            """Get status of all registered bots"""
            try:
                bots = await self.bot_manager.get_all_bots()
                return {"bots": bots, "count": len(bots)}
            except Exception as e:
                self.logger.error(f"Error getting bots: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.get("/api/bots/{bot_id}")
        async def get_bot_details(bot_id: str):
            """Get detailed information about a specific bot"""
            try:
                bot = await self.bot_manager.get_bot(bot_id)
                if not bot:
                    raise HTTPException(status_code=404, detail="Bot not found")
                
                # Get recent activity from database
                history = await self.db_manager.get_bot_history(bot_id, limit=50)
                
                return {"bot": bot, "history": history}
            except HTTPException:
                raise
            except Exception as e:
                self.logger.error(f"Error getting bot details: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.put("/api/bots/{bot_id}/name")
        async def update_bot_name(bot_id: str, data: dict):
            """Update the display name for a bot"""
            try:
                new_name = data.get("name", "").strip()
                if not new_name:
                    raise HTTPException(status_code=400, detail="Name cannot be empty")
                
                success = await self.bot_manager.update_bot_name(bot_id, new_name)
                if not success:
                    raise HTTPException(status_code=404, detail="Bot not found")
                
                self.logger.info(f"Bot {bot_id} renamed to: {new_name}")
                
                # Broadcast the update to all WebSocket connections
                await self.broadcast_to_websockets({
                    "type": "bot_name_update",
                    "bot_id": bot_id,
                    "name": new_name
                })
                
                return {"success": True, "message": f"Bot renamed to '{new_name}'"}
            except HTTPException:
                raise
            except Exception as e:
                self.logger.error(f"Error updating bot name: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.delete("/api/bots/cleanup")
        async def cleanup_inactive_bots(max_inactive_minutes: int = 5):
            """Remove bots that have been inactive for specified time"""
            try:
                removed_count = await self.bot_manager.remove_inactive_bots(max_inactive_minutes)
                
                self.logger.info(f"Cleaned up {removed_count} inactive bots")
                
                # Broadcast the update to all WebSocket connections
                await self.broadcast_to_websockets({
                    "type": "bots_cleaned_up",
                    "removed_count": removed_count
                })
                
                return {
                    "success": True, 
                    "message": f"Removed {removed_count} inactive bots",
                    "removed_count": removed_count
                }
            except Exception as e:
                self.logger.error(f"Error cleaning up bots: {e}")
                raise HTTPException(status_code=500, detail=str(e))
            except HTTPException:
                raise
            except Exception as e:
                self.logger.error(f"Error getting bot details: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.get("/api/esp-now/activity")
        async def get_esp_now_activity(limit: int = 100):
            """Get recent ESP-NOW activity"""
            try:
                activity = await self.db_manager.get_esp_now_activity(limit=limit)
                return {"activity": activity, "count": len(activity)}
            except Exception as e:
                self.logger.error(f"Error getting ESP-NOW activity: {e}")
                raise HTTPException(status_code=500, detail=str(e))

        @self.app.get("/api/firmware/latest")
        async def get_latest_firmware_version():
            """Get metadata for the latest available firmware."""
            if not self.firmware_version_file.exists():
                raise HTTPException(status_code=404, detail="No firmware version information available.")
            try:
                with open(self.firmware_version_file, 'r') as f:
                    version_info = json.load(f)
                return version_info
            except Exception as e:
                self.logger.error(f"Error reading firmware version file: {e}")
                raise HTTPException(status_code=500, detail="Could not retrieve firmware information.")

        @self.app.post("/api/firmware/upload", dependencies=[Depends(verify_api_key)])
        async def upload_firmware(
            version: float = Form(...),
            notes: str = Form(""),
            file: UploadFile = File(...)
        ):
            """Upload a new firmware binary."""
            if not file.filename.endswith(".bin"):
                raise HTTPException(status_code=400, detail="Invalid file type. Only .bin files are allowed.")

            try:
                # Save the binary file
                file_path = self.firmware_dir / file.filename
                with open(file_path, "wb") as buffer:
                    buffer.write(await file.read())
                
                # Update the version info file
                version_info = {
                    "version": version,
                    "filename": file.filename,
                    "notes": notes,
                    "upload_timestamp": datetime.now(timezone.utc).isoformat()
                }
                with open(self.firmware_version_file, 'w') as f:
                    json.dump(version_info, f, indent=2)

                self.logger.info(f"New firmware uploaded: {file.filename} (Version: {version})")
                
                # Optional: Broadcast a message to dashboards about the new firmware
                # await self._broadcast_system_message("new_firmware", version_info)

                return {"status": "success", "version": version, "filename": file.filename}
            except Exception as e:
                self.logger.error(f"Firmware upload failed: {e}")
                raise HTTPException(status_code=500, detail=f"Firmware upload failed: {e}")

        @self.app.websocket("/ws")
        async def websocket_endpoint(websocket: WebSocket):
            """WebSocket endpoint for real-time updates"""
            await websocket.accept()
            self.websocket_connections.append(websocket)
            self.logger.info("New WebSocket connection established")
            
            try:
                # Send initial data
                bots = await self.bot_manager.get_all_bots()
                await websocket.send_json({
                    "type": "initial_data",
                    "bots": bots
                })
                
                # Keep connection alive
                while True:
                    # Send periodic heartbeat/updates
                    await asyncio.sleep(5)
                    await websocket.send_json({
                        "type": "heartbeat",
                        "timestamp": datetime.now(timezone.utc).isoformat()
                    })
                    
            except WebSocketDisconnect:
                if websocket in self.websocket_connections:
                    self.websocket_connections.remove(websocket)
                self.logger.info("WebSocket connection closed")
            except Exception as e:
                self.logger.error(f"WebSocket error: {e}")
                if websocket in self.websocket_connections:
                    self.websocket_connections.remove(websocket)

    async def _broadcast_bot_update(self, bot_data: BotStatusData):
        """Broadcast bot status update to all WebSocket connections"""
        if not self.websocket_connections:
            return
            
        message = {
            "type": "bot_update",
            "bot_id": bot_data.bot_id,
            "data": bot_data.dict(),
            "timestamp": datetime.now(timezone.utc).isoformat()
        }
        
        # Remove disconnected connections
        active_connections = []
        for websocket in self.websocket_connections:
            try:
                await websocket.send_json(message)
                active_connections.append(websocket)
            except:
                self.logger.debug("Removed disconnected WebSocket")
                
        self.websocket_connections = active_connections

    async def _broadcast_esp_now_activity(self, esp_now_data: ESPNowMessage):
        """Broadcast ESP-NOW activity to all WebSocket connections"""
        if not self.websocket_connections:
            return
            
        message = {
            "type": "esp_now_activity",
            "data": esp_now_data.dict(),
            "timestamp": datetime.now(timezone.utc).isoformat()
        }
        
        # Remove disconnected connections
        active_connections = []
        for websocket in self.websocket_connections:
            try:
                await websocket.send_json(message)
                active_connections.append(websocket)
            except:
                self.logger.debug("Removed disconnected WebSocket")
                
        self.websocket_connections = active_connections

    async def broadcast_to_websockets(self, message: dict):
        """Broadcast a message to all WebSocket connections"""
        if not self.websocket_connections:
            return
            
        # Remove disconnected connections
        active_connections = []
        for websocket in self.websocket_connections:
            try:
                await websocket.send_json(message)
                active_connections.append(websocket)
            except:
                self.logger.debug("Removed disconnected WebSocket")
                
        self.websocket_connections = active_connections

    async def start_background_tasks(self):
        """Start background monitoring tasks"""
        self.logger.info("Starting background tasks")
        
        # Initialize database
        await self.db_manager.initialize()
        
        # Start health monitoring
        asyncio.create_task(self._health_monitor())
        
        # Start cleanup task
        asyncio.create_task(self._cleanup_task())

    async def _health_monitor(self):
        """Monitor bot health and detect timeouts"""
        interval = self.config.get("monitoring", {}).get("health_check_interval_seconds", 10)
        
        while True:
            try:
                await asyncio.sleep(interval)
                
                # Check for inactive bots
                inactive_bots = await self.bot_manager.check_bot_health()
                
                if inactive_bots:
                    self.logger.warning(f"Detected {len(inactive_bots)} inactive bots")
                    
                    # Broadcast health updates
                    for bot_id in inactive_bots:
                        await self._broadcast_bot_update(BotStatusData(
                            bot_id=bot_id,
                            status="inactive"
                        ))
                        
            except Exception as e:
                self.logger.error(f"Health monitor error: {e}")

    async def _cleanup_task(self):
        """Periodic cleanup of old data"""
        while True:
            try:
                # Run cleanup every hour
                await asyncio.sleep(3600)
                
                retention_days = self.config.get("monitoring", {}).get("data_retention_days", 30)
                cutoff_date = datetime.now(timezone.utc) - timedelta(days=retention_days)
                
                await self.db_manager.cleanup_old_data(cutoff_date)
                self.logger.info(f"Cleaned up data older than {retention_days} days")
                
            except Exception as e:
                self.logger.error(f"Cleanup task error: {e}")

    def _register_mdns_service(self, host, port):
        """Register the MCP server as an mDNS service."""
        try:
            self.zeroconf = Zeroconf()
            service_name = "mcp-server"
            service_type = "_mcp-server._tcp.local."
            
            self.service_info = ServiceInfo(
                service_type,
                f"{service_name}.{service_type}",
                addresses=[inet_aton(host)],
                port=port,
                properties={'version': '1.0'},
                server=f"{service_name}.local.",
            )
            self.zeroconf.register_service(self.service_info)
            self.logger.info(f"mDNS service '{service_name}' registered on {host}:{port}")
        except Exception as e:
            self.logger.error(f"Could not start mDNS service: {e}")

    def _unregister_mdns_service(self):
        if self.zeroconf and self.service_info:
            self.zeroconf.unregister_service(self.service_info)
            self.zeroconf.close()

    def _get_local_ip(self) -> str:
        """Get the local IP address of the machine."""
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # Doesn't have to be reachable
            s.connect(('10.255.255.255', 1))
            ip = s.getsockname()[0]
        except Exception:
            self.logger.warning("Could not determine local IP. Defaulting to 127.0.0.1")
            ip = '127.0.0.1'
        finally:
            s.close()
        return ip

    def run(self):
        """Start the MCP server"""
        server_config = self.config.get("server", {})
        host = server_config.get("host", "0.0.0.0")
        port = server_config.get("port", 8080)
        debug = server_config.get("debug", False)
        
        self.logger.info(f"Starting MCP Server on {host}:{port}")
        
        # Register mDNS service with the actual local IP
        local_ip = self._get_local_ip()
        self.logger.info(f"Advertising mDNS service on IP: {local_ip}")
        self._register_mdns_service(local_ip, port)

        uvicorn.run(
            self.app,
            host=host,
            port=port,
            log_level="debug" if debug else "info"
        )


def main():
    """Main entry point"""
    # Ensure data directory exists
    os.makedirs("data", exist_ok=True)
    os.makedirs("data/logs", exist_ok=True)
    
    # Start the server
    server = MCPServer()
    server.run()


if __name__ == "__main__":
    main()