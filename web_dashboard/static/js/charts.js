// Charts module for additional analytics and visualizations

class MCPCharts {
    constructor() {
        this.charts = new Map();
        this.initializeCharts();
    }

    initializeCharts() {
        // This will be called when additional chart containers are added
        console.log('MCP Charts initialized');
    }

    // Battery level distribution chart
    createBatteryDistributionChart(containerId, bots) {
        const ctx = document.getElementById(containerId).getContext('2d');
        
        // Group bots by battery level ranges
        const ranges = {
            'Critical (0-20%)': 0,
            'Low (21-40%)': 0,
            'Medium (41-70%)': 0,
            'High (71-100%)': 0,
            'Unknown': 0
        };

        bots.forEach(bot => {
            if (bot.battery_level === null) {
                ranges['Unknown']++;
            } else if (bot.battery_level <= 20) {
                ranges['Critical (0-20%)']++;
            } else if (bot.battery_level <= 40) {
                ranges['Low (21-40%)']++;
            } else if (bot.battery_level <= 70) {
                ranges['Medium (41-70%)']++;
            } else {
                ranges['High (71-100%)']++;
            }
        });

        const chart = new Chart(ctx, {
            type: 'doughnut',
            data: {
                labels: Object.keys(ranges),
                datasets: [{
                    data: Object.values(ranges),
                    backgroundColor: [
                        '#ff0040',  // Critical - Red
                        '#ffaa00',  // Low - Orange
                        '#66ccff',  // Medium - Blue
                        '#00ff41',  // High - Green
                        '#666666'   // Unknown - Gray
                    ],
                    borderColor: '#1a1a1a',
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            color: '#ffffff',
                            font: {
                                family: 'Courier New, monospace'
                            }
                        }
                    }
                }
            }
        });

        this.charts.set(containerId, chart);
        return chart;
    }

    // Activity timeline chart
    createActivityTimelineChart(containerId, activityData) {
        const ctx = document.getElementById(containerId).getContext('2d');
        
        // Group activity by hour
        const hourlyActivity = new Map();
        const now = new Date();
        
        // Initialize last 24 hours
        for (let i = 23; i >= 0; i--) {
            const hour = new Date(now.getTime() - (i * 60 * 60 * 1000));
            const hourKey = hour.getHours();
            hourlyActivity.set(hourKey, 0);
        }

        // Count messages per hour
        activityData.forEach(message => {
            const messageTime = new Date(message.timestamp);
            const hoursSinceMessage = (now - messageTime) / (1000 * 60 * 60);
            
            if (hoursSinceMessage <= 24) {
                const hour = messageTime.getHours();
                hourlyActivity.set(hour, (hourlyActivity.get(hour) || 0) + 1);
            }
        });

        const labels = Array.from(hourlyActivity.keys()).map(hour => 
            `${hour.toString().padStart(2, '0')}:00`
        );
        const data = Array.from(hourlyActivity.values());

        const chart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: labels,
                datasets: [{
                    label: 'ESP-NOW Messages',
                    data: data,
                    borderColor: '#00ff41',
                    backgroundColor: 'rgba(0, 255, 65, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        labels: {
                            color: '#ffffff',
                            font: {
                                family: 'Courier New, monospace'
                            }
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
                        beginAtZero: true,
                        ticks: {
                            color: '#ffffff',
                            stepSize: 1
                        },
                        grid: {
                            color: '#333333'
                        }
                    }
                }
            }
        });

        this.charts.set(containerId, chart);
        return chart;
    }

    // WiFi signal strength distribution
    createSignalStrengthChart(containerId, bots) {
        const ctx = document.getElementById(containerId).getContext('2d');
        
        // Group by signal strength ranges
        const ranges = {
            'Excellent (-30 to -50)': 0,
            'Good (-51 to -60)': 0,
            'Fair (-61 to -70)': 0,
            'Poor (-71 to -80)': 0,
            'Very Poor (-81+)': 0,
            'Unknown': 0
        };

        bots.forEach(bot => {
            if (bot.wifi_signal === null) {
                ranges['Unknown']++;
            } else if (bot.wifi_signal >= -50) {
                ranges['Excellent (-30 to -50)']++;
            } else if (bot.wifi_signal >= -60) {
                ranges['Good (-51 to -60)']++;
            } else if (bot.wifi_signal >= -70) {
                ranges['Fair (-61 to -70)']++;
            } else if (bot.wifi_signal >= -80) {
                ranges['Poor (-71 to -80)']++;
            } else {
                ranges['Very Poor (-81+)']++;
            }
        });

        const chart = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: Object.keys(ranges),
                datasets: [{
                    label: 'Bot Count',
                    data: Object.values(ranges),
                    backgroundColor: [
                        '#00ff41',  // Excellent - Green
                        '#66ccff',  // Good - Blue
                        '#ffaa00',  // Fair - Orange
                        '#ff6600',  // Poor - Red-Orange
                        '#ff0040',  // Very Poor - Red
                        '#666666'   // Unknown - Gray
                    ],
                    borderColor: '#1a1a1a',
                    borderWidth: 1
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        display: false
                    }
                },
                scales: {
                    x: {
                        ticks: {
                            color: '#ffffff',
                            maxRotation: 45
                        },
                        grid: {
                            color: '#333333'
                        }
                    },
                    y: {
                        beginAtZero: true,
                        ticks: {
                            color: '#ffffff',
                            stepSize: 1
                        },
                        grid: {
                            color: '#333333'
                        }
                    }
                }
            }
        });

        this.charts.set(containerId, chart);
        return chart;
    }

    // ESP-NOW message type distribution
    createMessageTypeChart(containerId, espNowActivity) {
        const ctx = document.getElementById(containerId).getContext('2d');
        
        // Count message types
        const messageTypes = {};
        espNowActivity.forEach(message => {
            messageTypes[message.message_type] = (messageTypes[message.message_type] || 0) + 1;
        });

        const chart = new Chart(ctx, {
            type: 'pie',
            data: {
                labels: Object.keys(messageTypes),
                datasets: [{
                    data: Object.values(messageTypes),
                    backgroundColor: [
                        '#00ff41',  // Green
                        '#ffaa00',  // Orange
                        '#66ccff',  // Blue
                        '#ff6600',  // Red-Orange
                        '#9966ff',  // Purple
                        '#ff0040'   // Red
                    ],
                    borderColor: '#1a1a1a',
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'right',
                        labels: {
                            color: '#ffffff',
                            font: {
                                family: 'Courier New, monospace'
                            }
                        }
                    }
                }
            }
        });

        this.charts.set(containerId, chart);
        return chart;
    }

    // Bot uptime chart
    createUptimeChart(containerId, bots) {
        const ctx = document.getElementById(containerId).getContext('2d');
        
        // Sort bots by uptime and take top 10
        const activeBots = bots
            .filter(bot => bot.uptime_seconds !== null && bot.is_active)
            .sort((a, b) => b.uptime_seconds - a.uptime_seconds)
            .slice(0, 10);

        const labels = activeBots.map(bot => bot.bot_id);
        const data = activeBots.map(bot => Math.floor(bot.uptime_seconds / 3600)); // Convert to hours

        const chart = new Chart(ctx, {
            type: 'horizontalBar',
            data: {
                labels: labels,
                datasets: [{
                    label: 'Uptime (hours)',
                    data: data,
                    backgroundColor: 'rgba(0, 255, 65, 0.2)',
                    borderColor: 'rgba(0, 255, 65, 1)',
                    borderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        labels: {
                            color: '#ffffff',
                            font: {
                                family: 'Courier New, monospace'
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        beginAtZero: true,
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
        });

        this.charts.set(containerId, chart);
        return chart;
    }

    // Update all charts with new data
    updateAllCharts(bots, espNowActivity) {
        // Destroy existing charts
        this.charts.forEach(chart => chart.destroy());
        this.charts.clear();

        // This method would be called when new chart containers are available
        // and we want to refresh all visualizations
    }

    // Destroy a specific chart
    destroyChart(containerId) {
        const chart = this.charts.get(containerId);
        if (chart) {
            chart.destroy();
            this.charts.delete(containerId);
        }
    }

    // Get chart instance
    getChart(containerId) {
        return this.charts.get(containerId);
    }

    // Export chart data as JSON
    exportChartData(containerId) {
        const chart = this.charts.get(containerId);
        if (chart) {
            return {
                type: chart.config.type,
                data: chart.data,
                timestamp: new Date().toISOString()
            };
        }
        return null;
    }

    // Generate chart as image (base64)
    exportChartImage(containerId) {
        const chart = this.charts.get(containerId);
        if (chart) {
            return chart.toBase64Image();
        }
        return null;
    }
}

// Initialize charts module
document.addEventListener('DOMContentLoaded', () => {
    window.mcpCharts = new MCPCharts();
});