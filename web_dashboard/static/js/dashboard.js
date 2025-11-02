// Master Control Program Dashboard JavaScript

class MCPDashboard {
    constructor() {
        this.ws = null;
        this.isConnected = false;
        this.bots = new Map();
        this.espNowActivity = [];
        this.currentChart = null;
        this.currentChartType = 'battery';
        
        this.init();
    }

    init() {
        this.setupWebSocket();
        this.setupEventListeners();
        this.loadInitialData();
        
        // Start periodic updates
        setInterval(() => this.refreshData(), 30000); // Every 30 seconds
    }

    setupWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;
        
        this.ws = new WebSocket(wsUrl);
        
        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.isConnected = true;
            this.updateConnectionStatus();
        };
        
        this.ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            this.handleWebSocketMessage(data);
        };
        
        this.ws.onclose = () => {
            console.log('WebSocket disconnected');
            this.isConnected = false;
            this.updateConnectionStatus();
            
            // Attempt to reconnect after 5 seconds
            setTimeout(() => this.setupWebSocket(), 5000);
        };
        
        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };
    }

    handleWebSocketMessage(data) {
        switch (data.type) {
            case 'initial_data':
                this.updateBotData(data.bots);
                break;
                
            case 'bot_update':
                this.handleBotUpdate(data);
                break;
                
            case 'bot_name_update':
                // Refresh the bot list when a name is updated
                this.refreshData();
                break;
                
            case 'bots_cleaned_up':
                // Refresh the bot list when bots are cleaned up
                this.refreshData();
                break;
                
            case 'esp_now_activity':
                this.handleESPNowActivity(data);
                break;
                
            case 'heartbeat':
                // Just acknowledge the heartbeat
                break;
                
            default:
                console.log('Unknown message type:', data.type);
        }
    }

    handleBotUpdate(data) {
        // Update or add bot to our local data
        const existingBots = Array.from(this.bots.values());
        const botIndex = existingBots.findIndex(bot => bot.bot_id === data.bot_id);
        
        if (botIndex >= 0) {
            // Update existing bot
            existingBots[botIndex] = { ...existingBots[botIndex], ...data.data };
        } else {
            // Add new bot
            existingBots.push(data.data);
        }
        
        this.updateBotData(existingBots);
    }

    handleESPNowActivity(data) {
        // Add new ESP-NOW activity to the beginning of the list
        this.espNowActivity.unshift(data.data);
        
        // Keep only the last 200 messages
        if (this.espNowActivity.length > 200) {
            this.espNowActivity = this.espNowActivity.slice(0, 200);
        }
        
        this.updateESPNowActivity();
    }

    updateConnectionStatus() {
        const statusDot = document.getElementById('connection-status');
        const statusText = document.getElementById('connection-text');
        
        if (this.isConnected) {
            statusDot.className = 'status-dot online';
            statusText.textContent = 'Connected';
        } else {
            statusDot.className = 'status-dot offline';
            statusText.textContent = 'Disconnected';
        }
    }

    async loadInitialData() {
        try {
            // Load bot data
            const botsResponse = await fetch('/api/bots');
            const botsData = await botsResponse.json();
            this.updateBotData(botsData.bots);
            
            // Load ESP-NOW activity
            const activityResponse = await fetch('/api/esp-now/activity?limit=100');
            const activityData = await activityResponse.json();
            this.espNowActivity = activityData.activity;
            this.updateESPNowActivity();
            
        } catch (error) {
            console.error('Error loading initial data:', error);
        }
    }

    updateBotData(bots) {
        // Update internal bot storage
        this.bots.clear();
        bots.forEach(bot => {
            this.bots.set(bot.bot_id, bot);
        });
        
        // Update statistics
        this.updateStatistics();
        
        // Update bot list
        this.updateBotList();
        
        // Update charts
        this.updateChart();
    }

    updateStatistics() {
        const bots = Array.from(this.bots.values());
        const activeBots = bots.filter(bot => bot.is_active);
        
        document.getElementById('total-bots').textContent = bots.length;
        document.getElementById('active-bots').textContent = activeBots.length;
        document.getElementById('esp-now-count').textContent = this.espNowActivity.length;
        
        // Calculate average battery
        const batteryLevels = activeBots
            .map(bot => bot.battery_level)
            .filter(level => level !== null);
        
        if (batteryLevels.length > 0) {
            const avgBattery = batteryLevels.reduce((sum, level) => sum + level, 0) / batteryLevels.length;
            document.getElementById('avg-battery').textContent = `${avgBattery.toFixed(1)}%`;
        } else {
            document.getElementById('avg-battery').textContent = '--';
        }
    }

    updateBotList() {
        const botList = document.getElementById('bot-list');
        const filter = document.getElementById('bot-filter').value;
        
        let bots = Array.from(this.bots.values());
        
        // Apply filter
        if (filter === 'active') {
            bots = bots.filter(bot => bot.is_active);
        } else if (filter === 'inactive') {
            bots = bots.filter(bot => !bot.is_active);
        }
        
        // Sort by last seen (most recent first)
        bots.sort((a, b) => new Date(b.last_seen) - new Date(a.last_seen));
        
        botList.innerHTML = '';
        
        if (bots.length === 0) {
            botList.innerHTML = '<div class="loading">No bots found</div>';
            return;
        }
        
        bots.forEach(bot => {
            const botItem = this.createBotListItem(bot);
            botList.appendChild(botItem);
        });
    }

    createBotListItem(bot) {
        const item = document.createElement('div');
        item.className = 'bot-item';
        item.onclick = () => this.showBotDetails(bot.bot_id);
        
        const statusClass = bot.is_active ? 'status-active' : 'status-inactive';
        const batteryClass = bot.battery_level < 20 ? 'status-warning' : '';
        
        // Use display name if available, otherwise use bot_id
        const displayName = bot.display_name || bot.bot_id;
        
        item.innerHTML = `
            <div class="bot-info">
                <div class="bot-id-container">
                    <div class="bot-id" title="Click to rename" onclick="event.stopPropagation(); window.mcpDashboard.renameBot('${bot.bot_id}', '${displayName}')">
                        ${displayName} ✏️
                    </div>
                    ${bot.display_name ? `<div class="bot-original-id">(${bot.bot_id})</div>` : ''}
                    ${bot.mac_address ? `<div class="bot-mac-address">MAC: ${bot.mac_address}</div>` : ''}
                </div>
                <div class="bot-status ${statusClass}">
                    ${bot.status} • Last seen: ${this.formatTimeAgo(bot.last_seen)}
                </div>
            </div>
            <div class="bot-metrics">
                ${bot.battery_level !== null ? 
                    `<div class="bot-battery ${batteryClass}">🔋 ${bot.battery_level}%</div>` : 
                    ''
                }
                ${bot.wifi_signal !== null ? 
                    `<div class="bot-signal">📶 ${bot.wifi_signal}dBm</div>` : 
                    ''
                }
                ${bot.uptime_seconds !== null ? 
                    `<div class="bot-uptime">⏱️ ${this.formatUptime(bot.uptime_seconds)}</div>` : 
                    ''
                }
            </div>
        `;
        
        return item;
    }

    updateESPNowActivity() {
        const activityList = document.getElementById('esp-now-activity');
        const filter = parseInt(document.getElementById('activity-filter').value);
        
        const messages = this.espNowActivity.slice(0, filter);
        
        activityList.innerHTML = '';
        
        if (messages.length === 0) {
            activityList.innerHTML = '<div class="loading">No ESP-NOW activity</div>';
            return;
        }
        
        messages.forEach(message => {
            const item = document.createElement('div');
            item.className = 'activity-item';
            
            // Add emoji indicators for message types
            let messageEmoji = '📡';
            if (message.message_type === 'heartbeat') messageEmoji = '💓';
            else if (message.message_type === 'status') messageEmoji = '📊';
            else if (message.message_type.endsWith('_ack')) messageEmoji = '✅';
            
            // Format sender/receiver names
            const senderName = this.getMacBotName(message.sender_mac);
            const receiverName = this.getMacBotName(message.receiver_mac);
            
            // Parse payload if it exists
            let payloadDisplay = '';
            if (message.payload && message.payload.data) {
                try {
                    // Try to parse JSON payload
                    const payloadData = typeof message.payload.data === 'string' 
                        ? JSON.parse(message.payload.data) 
                        : message.payload.data;
                    
                    if (payloadData.battery !== undefined) {
                        payloadDisplay = `<div class="activity-payload">🔋 ${payloadData.battery}% | 📶 ${payloadData.wifi_signal}dBm | ⏱️ ${payloadData.uptime}s</div>`;
                    } else if (typeof payloadData === 'string') {
                        payloadDisplay = `<div class="activity-payload">${payloadData}</div>`;
                    }
                } catch (e) {
                    payloadDisplay = `<div class="activity-payload">${message.payload.data}</div>`;
                }
            }
            
            item.innerHTML = `
                <div class="activity-timestamp">${this.formatTimestamp(message.timestamp)}</div>
                <div class="activity-macs">${messageEmoji} ${senderName} → ${receiverName}</div>
                <div class="activity-type">${message.message_type}</div>
                ${payloadDisplay}
                ${message.rssi ? `<div class="activity-rssi">RSSI: ${message.rssi}dBm</div>` : ''}
            `;
            
            activityList.appendChild(item);
        });
    }

    async showBotDetails(botId) {
        try {
            const response = await fetch(`/api/bots/${botId}`);
            const data = await response.json();
            
            const modal = document.getElementById('bot-modal');
            const modalBotId = document.getElementById('modal-bot-id');
            const modalContent = document.getElementById('bot-details-content');
            
            modalBotId.textContent = `Bot Details: ${botId}`;
            
            const bot = data.bot;
            const history = data.history;
            
            modalContent.innerHTML = `
                <div class="bot-details">
                    <h3>Current Status</h3>
                    <div class="detail-grid">
                        <div>Status: <span class="${bot.is_active ? 'status-active' : 'status-inactive'}">${bot.status}</span></div>
                        <div>Battery: ${bot.battery_level ? bot.battery_level + '%' : 'Unknown'}</div>
                        <div>WiFi Signal: ${bot.wifi_signal ? bot.wifi_signal + 'dBm' : 'Unknown'}</div>
                        <div>Uptime: ${bot.uptime_seconds ? this.formatUptime(bot.uptime_seconds) : 'Unknown'}</div>
                        <div>First Seen: ${this.formatTimestamp(bot.first_seen)}</div>
                        <div>Last Seen: ${this.formatTimestamp(bot.last_seen)}</div>
                        <div>Connection Count: ${bot.connection_count}</div>
                    </div>
                    
                    ${bot.location ? `
                        <h3>Location</h3>
                        <div>Lat: ${bot.location.lat}, Lng: ${bot.location.lng}</div>
                    ` : ''}
                    
                    ${bot.sensor_data ? `
                        <h3>Sensor Data</h3>
                        <pre>${JSON.stringify(bot.sensor_data, null, 2)}</pre>
                    ` : ''}
                    
                    <h3>Recent Activity</h3>
                    <div class="history-list">
                        ${history.map(record => `
                            <div class="history-item">
                                <div>${this.formatTimestamp(record.timestamp)} - ${record.status}</div>
                                ${record.battery_level ? `<div>Battery: ${record.battery_level}%</div>` : ''}
                            </div>
                        `).join('')}
                    </div>
                </div>
            `;
            
            modal.style.display = 'block';
            
        } catch (error) {
            console.error('Error loading bot details:', error);
        }
    }

    setupEventListeners() {
        // Bot filter
        document.getElementById('bot-filter').addEventListener('change', () => {
            this.updateBotList();
        });
        
        // Refresh button
        document.getElementById('refresh-bots').addEventListener('click', () => {
            this.refreshData();
        });
        
        // Cleanup inactive bots button
        document.getElementById('cleanup-bots').addEventListener('click', () => {
            this.cleanupInactiveBots();
        });
        
        // Activity filter
        document.getElementById('activity-filter').addEventListener('change', () => {
            this.updateESPNowActivity();
        });
        
        // Chart tabs
        document.querySelectorAll('.tab-button').forEach(button => {
            button.addEventListener('click', (e) => {
                document.querySelectorAll('.tab-button').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                this.currentChartType = e.target.dataset.chart;
                this.updateChart();
            });
        });
        
        // Modal close
        document.querySelector('.close').addEventListener('click', () => {
            document.getElementById('bot-modal').style.display = 'none';
        });
        
        // Close modal on outside click
        window.addEventListener('click', (e) => {
            const modal = document.getElementById('bot-modal');
            if (e.target === modal) {
                modal.style.display = 'none';
            }
        });
    }

    async refreshData() {
        try {
            const response = await fetch('/api/bots');
            const data = await response.json();
            this.updateBotData(data.bots);
        } catch (error) {
            console.error('Error refreshing data:', error);
        }
    }

    updateChart() {
        const ctx = document.getElementById('analytics-chart').getContext('2d');
        const bots = Array.from(this.bots.values()).filter(bot => bot.is_active);
        
        if (this.currentChart) {
            this.currentChart.destroy();
        }
        
        let chartData;
        let chartOptions;
        
        switch (this.currentChartType) {
            case 'battery':
                chartData = this.getBatteryChartData(bots);
                chartOptions = this.getChartOptions('Battery Levels (%)', 'bar');
                break;
            case 'activity':
                chartData = this.getActivityChartData(bots);
                chartOptions = this.getChartOptions('Connection Count', 'line');
                break;
            case 'signals':
                chartData = this.getSignalChartData(bots);
                chartOptions = this.getChartOptions('WiFi Signal (dBm)', 'bar');
                break;
        }
        
        this.currentChart = new Chart(ctx, {
            type: chartOptions.type,
            data: chartData,
            options: chartOptions.options
        });
    }

    getBatteryChartData(bots) {
        const labels = bots.map(bot => bot.bot_id);
        const data = bots.map(bot => bot.battery_level || 0);
        
        return {
            labels: labels,
            datasets: [{
                label: 'Battery Level',
                data: data,
                backgroundColor: 'rgba(0, 255, 65, 0.2)',
                borderColor: 'rgba(0, 255, 65, 1)',
                borderWidth: 2
            }]
        };
    }

    getActivityChartData(bots) {
        const labels = bots.map(bot => bot.bot_id);
        const data = bots.map(bot => bot.connection_count || 0);
        
        return {
            labels: labels,
            datasets: [{
                label: 'Connections',
                data: data,
                backgroundColor: 'rgba(0, 255, 65, 0.2)',
                borderColor: 'rgba(0, 255, 65, 1)',
                borderWidth: 2,
                fill: false
            }]
        };
    }

    getSignalChartData(bots) {
        const labels = bots.map(bot => bot.bot_id);
        const data = bots.map(bot => bot.wifi_signal || -100);
        
        return {
            labels: labels,
            datasets: [{
                label: 'WiFi Signal',
                data: data,
                backgroundColor: 'rgba(255, 170, 0, 0.2)',
                borderColor: 'rgba(255, 170, 0, 1)',
                borderWidth: 2
            }]
        };
    }

    getChartOptions(label, type) {
        return {
            type: type,
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        labels: {
                            color: '#ffffff'
                        }
                    }
                },
                scales: {
                    x: {
                        ticks: {
                            color: '#ffffff'
                        },
                        grid: {
                            color: '#333333'
                        }
                    },
                    y: {
                        ticks: {
                            color: '#ffffff'
                        },
                        grid: {
                            color: '#333333'
                        }
                    }
                }
            }
        };
    }

    // Bot renaming functionality
    async renameBot(botId, currentName) {
        const newName = prompt(`Enter new name for bot:`, currentName);
        if (!newName || newName.trim() === '' || newName === currentName) {
            return; // User cancelled or no change
        }
        
        try {
            const response = await fetch(`/api/bots/${botId}/name`, {
                method: 'PUT',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ name: newName.trim() })
            });
            
            if (response.ok) {
                const result = await response.json();
                console.log('Bot renamed successfully:', result.message);
                
                // The WebSocket will receive the update and refresh the UI
                // But we can also refresh immediately for better UX
                this.refreshData();
            } else {
                const error = await response.json();
                alert(`Failed to rename bot: ${error.detail}`);
            }
        } catch (error) {
            console.error('Error renaming bot:', error);
            alert('Failed to rename bot. Please try again.');
        }
    }

    // Bot cleanup functionality
    async cleanupInactiveBots() {
        if (!confirm('Remove bots that have been inactive for more than 5 minutes?')) {
            return;
        }
        
        try {
            const response = await fetch('/api/bots/cleanup', {
                method: 'DELETE'
            });
            
            if (response.ok) {
                const result = await response.json();
                alert(`${result.message}`);
                
                // Refresh the data immediately
                this.refreshData();
            } else {
                const error = await response.json();
                alert(`Failed to cleanup bots: ${error.detail}`);
            }
        } catch (error) {
            console.error('Error cleaning up bots:', error);
            alert('Failed to cleanup bots. Please try again.');
        }
    }

    formatTimeAgo(timestamp) {
        const now = new Date();
        const then = new Date(timestamp);
        const diffMs = now - then;
        const diffSecs = Math.floor(diffMs / 1000);
        
        if (diffSecs < 60) return `${diffSecs}s ago`;
        if (diffSecs < 3600) return `${Math.floor(diffSecs / 60)}m ago`;
        if (diffSecs < 86400) return `${Math.floor(diffSecs / 3600)}h ago`;
        return `${Math.floor(diffSecs / 86400)}d ago`;
    }

    formatTimestamp(timestamp) {
        return new Date(timestamp).toLocaleString();
    }

    formatUptime(seconds) {
        const days = Math.floor(seconds / 86400);
        const hours = Math.floor((seconds % 86400) / 3600);
        const mins = Math.floor((seconds % 3600) / 60);
        
        if (days > 0) return `${days}d ${hours}h`;
        if (hours > 0) return `${hours}h ${mins}m`;
        return `${mins}m`;
    }

    getMacBotName(macAddress) {
        // Convert MAC address to friendly bot name
        for (const [botId, bot] of this.bots.entries()) {
            if (bot.mac_address && bot.mac_address.toLowerCase() === macAddress.toLowerCase()) {
                return botId;
            }
        }
        
        // Return shortened MAC if no bot name found
        return macAddress.replace(/:/g, '').slice(-6).toUpperCase();
    }
}

// Initialize dashboard when page loads
document.addEventListener('DOMContentLoaded', () => {
    window.mcpDashboard = new MCPDashboard();
});