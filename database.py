"""
Database Manager - Handles SQLite database operations for bot data and ESP-NOW messages
"""

import asyncio
import aiosqlite
import json
import logging
from datetime import datetime, timedelta
from pathlib import Path
from typing import Dict, List, Optional


class DatabaseManager:
    def __init__(self, db_path: str):
        self.db_path = db_path
        self.logger = logging.getLogger(__name__)
        
        # Ensure database directory exists
        Path(db_path).parent.mkdir(parents=True, exist_ok=True)

    async def initialize(self):
        """Initialize database tables"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                # Bot status table
                await db.execute("""
                    CREATE TABLE IF NOT EXISTS bot_status (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        bot_id TEXT NOT NULL,
                        timestamp DATETIME NOT NULL,
                        status TEXT NOT NULL,
                        battery_level REAL,
                        wifi_signal INTEGER,
                        location TEXT,
                        sensor_data TEXT,
                        uptime_seconds INTEGER,
                        FOREIGN KEY (bot_id) REFERENCES bots (bot_id)
                    )
                """)
                
                # Bots table for registration info
                await db.execute("""
                    CREATE TABLE IF NOT EXISTS bots (
                        bot_id TEXT PRIMARY KEY,
                        first_seen DATETIME NOT NULL,
                        last_seen DATETIME NOT NULL,
                        connection_count INTEGER DEFAULT 0
                    )
                """)
                
                # ESP-NOW messages table
                await db.execute("""
                    CREATE TABLE IF NOT EXISTS esp_now_messages (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp DATETIME NOT NULL,
                        sender_mac TEXT NOT NULL,
                        receiver_mac TEXT NOT NULL,
                        message_type TEXT NOT NULL,
                        payload TEXT NOT NULL,
                        rssi INTEGER
                    )
                """)
                
                # System events table
                await db.execute("""
                    CREATE TABLE IF NOT EXISTS system_events (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        timestamp DATETIME NOT NULL,
                        event_type TEXT NOT NULL,
                        description TEXT,
                        data TEXT
                    )
                """)
                
                # Create indexes for better performance
                await db.execute("CREATE INDEX IF NOT EXISTS idx_bot_status_timestamp ON bot_status (timestamp)")
                await db.execute("CREATE INDEX IF NOT EXISTS idx_bot_status_bot_id ON bot_status (bot_id)")
                await db.execute("CREATE INDEX IF NOT EXISTS idx_esp_now_timestamp ON esp_now_messages (timestamp)")
                await db.execute("CREATE INDEX IF NOT EXISTS idx_esp_now_sender ON esp_now_messages (sender_mac)")
                await db.execute("CREATE INDEX IF NOT EXISTS idx_esp_now_receiver ON esp_now_messages (receiver_mac)")
                
                await db.commit()
                self.logger.info("Database initialized successfully")
                
        except Exception as e:
            self.logger.error(f"Database initialization error: {e}")
            raise

    async def store_bot_status(self, bot_data) -> None:
        """Store bot status data in database"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                # First, update or insert bot registration
                await db.execute("""
                    INSERT OR REPLACE INTO bots (bot_id, first_seen, last_seen, connection_count)
                    VALUES (?, 
                           COALESCE((SELECT first_seen FROM bots WHERE bot_id = ?), ?),
                           ?,
                           COALESCE((SELECT connection_count FROM bots WHERE bot_id = ?), 0) + 1)
                """, (
                    bot_data.bot_id,
                    bot_data.bot_id,
                    bot_data.timestamp,
                    bot_data.timestamp,
                    bot_data.bot_id
                ))
                
                # Store status data
                await db.execute("""
                    INSERT INTO bot_status 
                    (bot_id, timestamp, status, battery_level, wifi_signal, location, sensor_data, uptime_seconds)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)
                """, (
                    bot_data.bot_id,
                    bot_data.timestamp,
                    bot_data.status,
                    bot_data.battery_level,
                    bot_data.wifi_signal,
                    json.dumps(bot_data.location) if bot_data.location else None,
                    json.dumps(bot_data.sensor_data) if bot_data.sensor_data else None,
                    bot_data.uptime_seconds
                ))
                
                await db.commit()
                self.logger.debug(f"Stored status for bot {bot_data.bot_id}")
                
        except Exception as e:
            self.logger.error(f"Error storing bot status: {e}")
            raise

    async def store_esp_now_message(self, esp_now_data) -> None:
        """Store ESP-NOW message data"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                await db.execute("""
                    INSERT INTO esp_now_messages 
                    (timestamp, sender_mac, receiver_mac, message_type, payload, rssi)
                    VALUES (?, ?, ?, ?, ?, ?)
                """, (
                    esp_now_data.timestamp,
                    esp_now_data.sender_mac,
                    esp_now_data.receiver_mac,
                    esp_now_data.message_type,
                    json.dumps(esp_now_data.payload),
                    esp_now_data.rssi
                ))
                
                await db.commit()
                self.logger.debug(f"Stored ESP-NOW message: {esp_now_data.sender_mac} -> {esp_now_data.receiver_mac}")
                
        except Exception as e:
            self.logger.error(f"Error storing ESP-NOW message: {e}")
            raise

    async def get_bot_history(self, bot_id: str, limit: int = 50) -> List[Dict]:
        """Get recent status history for a specific bot"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                db.row_factory = aiosqlite.Row
                
                async with db.execute("""
                    SELECT * FROM bot_status 
                    WHERE bot_id = ? 
                    ORDER BY timestamp DESC 
                    LIMIT ?
                """, (bot_id, limit)) as cursor:
                    
                    rows = await cursor.fetchall()
                    history = []
                    
                    for row in rows:
                        record = {
                            "timestamp": row["timestamp"],
                            "status": row["status"],
                            "battery_level": row["battery_level"],
                            "wifi_signal": row["wifi_signal"],
                            "uptime_seconds": row["uptime_seconds"]
                        }
                        
                        # Parse JSON fields
                        if row["location"]:
                            record["location"] = json.loads(row["location"])
                        if row["sensor_data"]:
                            record["sensor_data"] = json.loads(row["sensor_data"])
                            
                        history.append(record)
                    
                    return history
                    
        except Exception as e:
            self.logger.error(f"Error getting bot history: {e}")
            return []

    async def get_esp_now_activity(self, limit: int = 100, bot_mac: Optional[str] = None) -> List[Dict]:
        """Get recent ESP-NOW activity"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                db.row_factory = aiosqlite.Row
                
                if bot_mac:
                    query = """
                        SELECT * FROM esp_now_messages 
                        WHERE sender_mac = ? OR receiver_mac = ?
                        ORDER BY timestamp DESC 
                        LIMIT ?
                    """
                    params = (bot_mac, bot_mac, limit)
                else:
                    query = """
                        SELECT * FROM esp_now_messages 
                        ORDER BY timestamp DESC 
                        LIMIT ?
                    """
                    params = (limit,)
                
                async with db.execute(query, params) as cursor:
                    rows = await cursor.fetchall()
                    activity = []
                    
                    for row in rows:
                        record = {
                            "timestamp": row["timestamp"],
                            "sender_mac": row["sender_mac"],
                            "receiver_mac": row["receiver_mac"],
                            "message_type": row["message_type"],
                            "payload": json.loads(row["payload"]) if row["payload"] else {},
                            "rssi": row["rssi"]
                        }
                        activity.append(record)
                    
                    return activity
                    
        except Exception as e:
            self.logger.error(f"Error getting ESP-NOW activity: {e}")
            return []

    async def get_bot_statistics(self, hours: int = 24) -> Dict:
        """Get bot statistics for the specified time period"""
        try:
            cutoff_time = datetime.now() - timedelta(hours=hours)
            
            async with aiosqlite.connect(self.db_path) as db:
                # Total unique bots
                async with db.execute("SELECT COUNT(DISTINCT bot_id) FROM bots") as cursor:
                    total_bots = (await cursor.fetchone())[0]
                
                # Active bots in time period
                async with db.execute("""
                    SELECT COUNT(DISTINCT bot_id) FROM bot_status 
                    WHERE timestamp > ?
                """, (cutoff_time,)) as cursor:
                    active_bots = (await cursor.fetchone())[0]
                
                # Message counts
                async with db.execute("""
                    SELECT COUNT(*) FROM bot_status WHERE timestamp > ?
                """, (cutoff_time,)) as cursor:
                    status_messages = (await cursor.fetchone())[0]
                
                async with db.execute("""
                    SELECT COUNT(*) FROM esp_now_messages WHERE timestamp > ?
                """, (cutoff_time,)) as cursor:
                    esp_now_messages = (await cursor.fetchone())[0]
                
                # Average battery level (recent)
                async with db.execute("""
                    SELECT AVG(battery_level) FROM bot_status 
                    WHERE timestamp > ? AND battery_level IS NOT NULL
                """, (cutoff_time,)) as cursor:
                    avg_battery = (await cursor.fetchone())[0]
                
                return {
                    "period_hours": hours,
                    "total_bots": total_bots,
                    "active_bots": active_bots,
                    "status_messages": status_messages,
                    "esp_now_messages": esp_now_messages,
                    "average_battery": round(avg_battery, 2) if avg_battery else None,
                    "timestamp": datetime.now().isoformat()
                }
                
        except Exception as e:
            self.logger.error(f"Error getting bot statistics: {e}")
            return {}

    async def get_esp_now_network_graph(self, hours: int = 24) -> Dict:
        """Generate network graph data from ESP-NOW communications"""
        try:
            cutoff_time = datetime.now() - timedelta(hours=hours)
            
            async with aiosqlite.connect(self.db_path) as db:
                db.row_factory = aiosqlite.Row
                
                # Get unique MAC addresses and their connections
                async with db.execute("""
                    SELECT sender_mac, receiver_mac, COUNT(*) as message_count,
                           MAX(timestamp) as last_message
                    FROM esp_now_messages 
                    WHERE timestamp > ?
                    GROUP BY sender_mac, receiver_mac
                    ORDER BY message_count DESC
                """, (cutoff_time,)) as cursor:
                    
                    connections = await cursor.fetchall()
                    
                    # Build nodes and edges
                    nodes = set()
                    edges = []
                    
                    for conn in connections:
                        nodes.add(conn["sender_mac"])
                        nodes.add(conn["receiver_mac"])
                        
                        edges.append({
                            "source": conn["sender_mac"],
                            "target": conn["receiver_mac"],
                            "message_count": conn["message_count"],
                            "last_message": conn["last_message"]
                        })
                    
                    # Convert nodes to list with additional info
                    node_list = []
                    for mac in nodes:
                        # Get recent activity for this node
                        async with db.execute("""
                            SELECT COUNT(*) as sent_count FROM esp_now_messages 
                            WHERE sender_mac = ? AND timestamp > ?
                        """, (mac, cutoff_time)) as cursor:
                            sent_count = (await cursor.fetchone())[0]
                        
                        async with db.execute("""
                            SELECT COUNT(*) as received_count FROM esp_now_messages 
                            WHERE receiver_mac = ? AND timestamp > ?
                        """, (mac, cutoff_time)) as cursor:
                            received_count = (await cursor.fetchone())[0]
                        
                        node_list.append({
                            "id": mac,
                            "sent_count": sent_count,
                            "received_count": received_count,
                            "total_activity": sent_count + received_count
                        })
                    
                    return {
                        "nodes": node_list,
                        "edges": edges,
                        "period_hours": hours,
                        "timestamp": datetime.now().isoformat()
                    }
                    
        except Exception as e:
            self.logger.error(f"Error generating network graph: {e}")
            return {"nodes": [], "edges": []}

    async def cleanup_old_data(self, cutoff_date: datetime) -> Dict[str, int]:
        """Remove old data to manage database size"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                # Count records to be deleted
                async with db.execute("""
                    SELECT COUNT(*) FROM bot_status WHERE timestamp < ?
                """, (cutoff_date,)) as cursor:
                    old_status_count = (await cursor.fetchone())[0]
                
                async with db.execute("""
                    SELECT COUNT(*) FROM esp_now_messages WHERE timestamp < ?
                """, (cutoff_date,)) as cursor:
                    old_esp_now_count = (await cursor.fetchone())[0]
                
                # Delete old records
                await db.execute("DELETE FROM bot_status WHERE timestamp < ?", (cutoff_date,))
                await db.execute("DELETE FROM esp_now_messages WHERE timestamp < ?", (cutoff_date,))
                
                # Clean up bots that haven't been seen
                await db.execute("DELETE FROM bots WHERE last_seen < ?", (cutoff_date,))
                
                await db.commit()
                
                # Vacuum to reclaim space
                await db.execute("VACUUM")
                
                self.logger.info(f"Cleaned up {old_status_count} status records and {old_esp_now_count} ESP-NOW records")
                
                return {
                    "status_records_deleted": old_status_count,
                    "esp_now_records_deleted": old_esp_now_count,
                    "cutoff_date": cutoff_date.isoformat()
                }
                
        except Exception as e:
            self.logger.error(f"Error during cleanup: {e}")
            return {}

    async def log_system_event(self, event_type: str, description: str, data: Optional[Dict] = None):
        """Log system events for debugging and monitoring"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                await db.execute("""
                    INSERT INTO system_events (timestamp, event_type, description, data)
                    VALUES (?, ?, ?, ?)
                """, (
                    datetime.now(),
                    event_type,
                    description,
                    json.dumps(data) if data else None
                ))
                await db.commit()
                
        except Exception as e:
            self.logger.error(f"Error logging system event: {e}")

    async def get_database_info(self) -> Dict:
        """Get database size and record counts"""
        try:
            async with aiosqlite.connect(self.db_path) as db:
                # Get table sizes
                tables = ["bots", "bot_status", "esp_now_messages", "system_events"]
                table_info = {}
                
                for table in tables:
                    async with db.execute(f"SELECT COUNT(*) FROM {table}") as cursor:
                        count = (await cursor.fetchone())[0]
                        table_info[table] = count
                
                # Get database file size
                db_size = Path(self.db_path).stat().st_size if Path(self.db_path).exists() else 0
                
                return {
                    "database_path": self.db_path,
                    "database_size_bytes": db_size,
                    "database_size_mb": round(db_size / (1024 * 1024), 2),
                    "table_counts": table_info,
                    "timestamp": datetime.now().isoformat()
                }
                
        except Exception as e:
            self.logger.error(f"Error getting database info: {e}")
            return {}