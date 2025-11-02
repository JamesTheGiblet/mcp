"""
Bot Manager - Handles bot registration, status tracking, and health monitoring
"""

import asyncio
import logging
from datetime import datetime, timedelta
from typing import Dict, List, Optional

from pydantic import BaseModel


class BotStatus(BaseModel):
    bot_id: str
    display_name: Optional[str] = None  # Custom display name for the bot
    mac_address: Optional[str] = None  # MAC address of the bot
    last_seen: datetime
    status: str
    battery_level: Optional[float] = None
    wifi_signal: Optional[int] = None
    location: Optional[Dict[str, float]] = None
    sensor_data: Optional[Dict] = None
    uptime_seconds: Optional[int] = None
    connection_count: int = 0
    first_seen: datetime


class BotManager:
    def __init__(self, config: Dict):
        self.config = config
        self.bots: Dict[str, BotStatus] = {}
        self.logger = logging.getLogger(__name__)
        self.bot_timeout = config.get("monitoring", {}).get("bot_timeout_seconds", 30)

    async def update_bot_status(self, bot_data) -> None:
        """Update bot status with new data"""
        bot_id = bot_data.bot_id
        current_time = datetime.now()
        
        if bot_id in self.bots:
            # Update existing bot
            bot = self.bots[bot_id]
            bot.last_seen = current_time
            bot.status = bot_data.status
            bot.connection_count += 1
            
            # Update optional fields if provided
            if bot_data.battery_level is not None:
                bot.battery_level = bot_data.battery_level
            if bot_data.wifi_signal is not None:
                bot.wifi_signal = bot_data.wifi_signal
            if bot_data.mac_address is not None:
                bot.mac_address = bot_data.mac_address
            if bot_data.location is not None:
                bot.location = bot_data.location
            if bot_data.sensor_data is not None:
                bot.sensor_data = bot_data.sensor_data
            if bot_data.uptime_seconds is not None:
                bot.uptime_seconds = bot_data.uptime_seconds
                
            self.logger.debug(f"Updated bot {bot_id} status to {bot_data.status}")
        else:
            # Register new bot
            self.bots[bot_id] = BotStatus(
                bot_id=bot_id,
                mac_address=bot_data.mac_address,
                first_seen=current_time,
                last_seen=current_time,
                status=bot_data.status,
                battery_level=bot_data.battery_level,
                wifi_signal=bot_data.wifi_signal,
                location=bot_data.location,
                sensor_data=bot_data.sensor_data,
                uptime_seconds=bot_data.uptime_seconds,
                connection_count=1
            )
            self.logger.info(f"Registered new bot: {bot_id}")

    async def get_bot(self, bot_id: str) -> Optional[BotStatus]:
        """Get bot status by ID"""
        return self.bots.get(bot_id)

    async def update_bot_name(self, bot_id: str, new_name: str) -> bool:
        """Update the display name for a bot"""
        if bot_id not in self.bots:
            return False
        
        self.bots[bot_id].display_name = new_name
        self.logger.info(f"Updated bot {bot_id} display name to: {new_name}")
        return True

    async def get_all_bots(self) -> List[Dict]:
        """Get all registered bots with their status"""
        current_time = datetime.now()
        bot_list = []
        
        for bot_id, bot in self.bots.items():
            # Calculate time since last seen
            time_since_last_seen = (current_time - bot.last_seen).total_seconds()
            
            # Determine if bot is active
            is_active = time_since_last_seen <= self.bot_timeout
            
            bot_info = {
                "bot_id": bot.bot_id,
                "display_name": bot.display_name,
                "status": bot.status if is_active else "inactive",
                "last_seen": bot.last_seen.isoformat(),
                "first_seen": bot.first_seen.isoformat(),
                "battery_level": bot.battery_level,
                "wifi_signal": bot.wifi_signal,
                "location": bot.location,
                "sensor_data": bot.sensor_data,
                "uptime_seconds": bot.uptime_seconds,
                "connection_count": bot.connection_count,
                "time_since_last_seen": int(time_since_last_seen),
                "is_active": is_active
            }
            bot_list.append(bot_info)
        
        # Sort by last seen (most recent first)
        bot_list.sort(key=lambda x: x["last_seen"], reverse=True)
        return bot_list

    async def check_bot_health(self) -> List[str]:
        """Check bot health and return list of inactive bot IDs"""
        current_time = datetime.now()
        inactive_bots = []
        
        for bot_id, bot in self.bots.items():
            time_since_last_seen = (current_time - bot.last_seen).total_seconds()
            
            if time_since_last_seen > self.bot_timeout and bot.status != "inactive":
                bot.status = "inactive"
                inactive_bots.append(bot_id)
                self.logger.warning(
                    f"Bot {bot_id} marked as inactive "
                    f"(last seen {int(time_since_last_seen)}s ago)"
                )
        
        return inactive_bots

    async def get_bot_statistics(self) -> Dict:
        """Get overall bot network statistics"""
        current_time = datetime.now()
        total_bots = len(self.bots)
        active_bots = 0
        inactive_bots = 0
        
        battery_levels = []
        wifi_signals = []
        
        for bot in self.bots.values():
            time_since_last_seen = (current_time - bot.last_seen).total_seconds()
            
            if time_since_last_seen <= self.bot_timeout:
                active_bots += 1
                
                if bot.battery_level is not None:
                    battery_levels.append(bot.battery_level)
                if bot.wifi_signal is not None:
                    wifi_signals.append(bot.wifi_signal)
            else:
                inactive_bots += 1
        
        stats = {
            "total_bots": total_bots,
            "active_bots": active_bots,
            "inactive_bots": inactive_bots,
            "activity_rate": (active_bots / total_bots * 100) if total_bots > 0 else 0,
            "average_battery": sum(battery_levels) / len(battery_levels) if battery_levels else None,
            "average_wifi_signal": sum(wifi_signals) / len(wifi_signals) if wifi_signals else None,
            "low_battery_count": sum(1 for level in battery_levels if level < 20) if battery_levels else 0
        }
        
        return stats

    async def remove_old_bots(self, max_age_days: int = 7) -> int:
        """Remove bots that haven't been seen for a long time"""
        cutoff_time = datetime.now() - timedelta(days=max_age_days)
        removed_count = 0
        
        bots_to_remove = [
            bot_id for bot_id, bot in self.bots.items()
            if bot.last_seen < cutoff_time
        ]
        
        for bot_id in bots_to_remove:
            del self.bots[bot_id]
            removed_count += 1
            self.logger.info(f"Removed old bot: {bot_id}")
        
        return removed_count

    async def remove_inactive_bots(self, max_inactive_minutes: int = 5) -> int:
        """Remove bots that have been inactive for a specified time"""
        current_time = datetime.now()
        cutoff_time = current_time - timedelta(minutes=max_inactive_minutes)
        removed_count = 0
        
        bots_to_remove = []
        for bot_id, bot in self.bots.items():
            time_since_last_seen = (current_time - bot.last_seen).total_seconds()
            if time_since_last_seen > (max_inactive_minutes * 60):
                bots_to_remove.append(bot_id)
        
        for bot_id in bots_to_remove:
            del self.bots[bot_id]
            removed_count += 1
            self.logger.info(f"Removed inactive bot: {bot_id}")
        
        return removed_count

    async def get_bot_locations(self) -> List[Dict]:
        """Get locations of all bots that have location data"""
        locations = []
        
        for bot in self.bots.values():
            if bot.location:
                current_time = datetime.now()
                time_since_last_seen = (current_time - bot.last_seen).total_seconds()
                is_active = time_since_last_seen <= self.bot_timeout
                
                location_info = {
                    "bot_id": bot.bot_id,
                    "location": bot.location,
                    "status": bot.status if is_active else "inactive",
                    "last_seen": bot.last_seen.isoformat(),
                    "is_active": is_active
                }
                locations.append(location_info)
        
        return locations

    async def get_network_topology(self) -> Dict:
        """
        Generate network topology data for visualization
        This would be enhanced with actual ESP-NOW communication data
        """
        nodes = []
        edges = []
        
        # Add MCP as central node
        nodes.append({
            "id": "MCP",
            "type": "master",
            "label": "Master Control Program",
            "status": "active"
        })
        
        # Add bot nodes
        for bot in self.bots.values():
            current_time = datetime.now()
            time_since_last_seen = (current_time - bot.last_seen).total_seconds()
            is_active = time_since_last_seen <= self.bot_timeout
            
            nodes.append({
                "id": bot.bot_id,
                "type": "bot",
                "label": bot.bot_id,
                "status": "active" if is_active else "inactive",
                "battery_level": bot.battery_level,
                "wifi_signal": bot.wifi_signal
            })
            
            # Add edge from bot to MCP (WiFi connection)
            edges.append({
                "source": bot.bot_id,
                "target": "MCP",
                "type": "wifi",
                "active": is_active
            })
        
        return {
            "nodes": nodes,
            "edges": edges,
            "timestamp": datetime.now().isoformat()
        }