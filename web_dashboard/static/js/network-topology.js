// Network Topology Visualization using D3.js

class NetworkTopology {
    constructor(containerId) {
        this.container = d3.select(`#${containerId}`);
        this.svg = this.container.select('svg');
        this.width = 0;
        this.height = 0;
        this.simulation = null;
        this.nodes = [];
        this.links = [];
        
        this.init();
    }

    init() {
        this.updateDimensions();
        this.setupSVG();
        
        // Update dimensions on window resize
        window.addEventListener('resize', () => {
            this.updateDimensions();
            this.resize();
        });
    }

    updateDimensions() {
        const rect = this.container.node().getBoundingClientRect();
        this.width = rect.width - 40; // Account for padding
        this.height = rect.height - 40;
    }

    setupSVG() {
        this.svg
            .attr('width', this.width)
            .attr('height', this.height);

        // Create groups for different elements
        this.linksGroup = this.svg.append('g').attr('class', 'links');
        this.nodesGroup = this.svg.append('g').attr('class', 'nodes');

        // Setup zoom behavior
        const zoom = d3.zoom()
            .scaleExtent([0.1, 4])
            .on('zoom', (event) => {
                this.linksGroup.attr('transform', event.transform);
                this.nodesGroup.attr('transform', event.transform);
            });

        this.svg.call(zoom);

        // Create force simulation
        this.simulation = d3.forceSimulation()
            .force('link', d3.forceLink().id(d => d.id).distance(100))
            .force('charge', d3.forceManyBody().strength(-300))
            .force('center', d3.forceCenter(this.width / 2, this.height / 2))
            .force('collision', d3.forceCollide().radius(30));
    }

    updateNetwork(networkData) {
        this.nodes = networkData.nodes || [];
        this.links = networkData.edges || [];

        this.updateLinks();
        this.updateNodes();
        this.updateSimulation();
    }

    updateLinks() {
        const link = this.linksGroup
            .selectAll('line')
            .data(this.links);

        link.exit().remove();

        const linkEnter = link.enter()
            .append('line')
            .attr('stroke-width', 2)
            .attr('stroke-opacity', 0.6);

        linkEnter.merge(link)
            .attr('stroke', d => {
                switch (d.type) {
                    case 'wifi': return '#00ff41';
                    case 'esp_now': return '#ffaa00';
                    default: return '#666666';
                }
            })
            .attr('stroke-dasharray', d => d.active ? null : '5,5');
    }

    updateNodes() {
        const node = this.nodesGroup
            .selectAll('.node')
            .data(this.nodes);

        node.exit().remove();

        const nodeEnter = node.enter()
            .append('g')
            .attr('class', 'node')
            .call(this.drag());

        // Add circles for nodes
        nodeEnter.append('circle')
            .attr('r', 20);

        // Add labels
        nodeEnter.append('text')
            .attr('text-anchor', 'middle')
            .attr('dy', '.35em')
            .style('font-size', '10px')
            .style('font-family', 'Courier New, monospace')
            .style('fill', '#ffffff')
            .style('pointer-events', 'none');

        // Add battery indicator for bots
        nodeEnter.append('circle')
            .attr('class', 'battery-indicator')
            .attr('r', 4)
            .attr('cx', 15)
            .attr('cy', -15)
            .style('opacity', 0);

        // Update existing nodes
        const nodeUpdate = nodeEnter.merge(node);

        nodeUpdate.select('circle')
            .attr('fill', d => this.getNodeColor(d))
            .attr('stroke', d => this.getNodeStrokeColor(d))
            .attr('stroke-width', d => d.type === 'master' ? 4 : 2);

        nodeUpdate.select('text')
            .text(d => this.getNodeLabel(d));

        // Update battery indicators
        nodeUpdate.select('.battery-indicator')
            .style('opacity', d => d.battery_level !== undefined ? 1 : 0)
            .attr('fill', d => this.getBatteryColor(d.battery_level));

        // Add tooltips
        nodeUpdate
            .on('mouseover', (event, d) => this.showTooltip(event, d))
            .on('mouseout', () => this.hideTooltip());
    }

    getNodeColor(d) {
        if (d.type === 'master') {
            return '#003d0a';
        }
        
        switch (d.status) {
            case 'active': return '#1a5a1a';
            case 'inactive': return '#5a1a1a';
            default: return '#3a3a3a';
        }
    }

    getNodeStrokeColor(d) {
        if (d.type === 'master') {
            return '#00ff41';
        }
        
        switch (d.status) {
            case 'active': return '#00ff41';
            case 'inactive': return '#ff0040';
            default: return '#666666';
        }
    }

    getNodeLabel(d) {
        if (d.type === 'master') {
            return 'MCP';
        }
        
        return d.label || d.id;
    }

    getBatteryColor(batteryLevel) {
        if (batteryLevel === undefined) return '#666666';
        if (batteryLevel > 50) return '#00ff41';
        if (batteryLevel > 20) return '#ffaa00';
        return '#ff0040';
    }

    showTooltip(event, d) {
        // Remove existing tooltip
        d3.select('.network-tooltip').remove();

        const tooltip = d3.select('body')
            .append('div')
            .attr('class', 'network-tooltip')
            .style('position', 'absolute')
            .style('background', '#1a1a1a')
            .style('border', '1px solid #00ff41')
            .style('border-radius', '4px')
            .style('padding', '8px')
            .style('font-family', 'Courier New, monospace')
            .style('font-size', '12px')
            .style('color', '#ffffff')
            .style('pointer-events', 'none')
            .style('z-index', '1000');

        let content = `<strong>${d.label || d.id}</strong><br/>`;
        content += `Type: ${d.type}<br/>`;
        content += `Status: ${d.status}<br/>`;
        
        if (d.battery_level !== undefined) {
            content += `Battery: ${d.battery_level}%<br/>`;
        }
        
        if (d.wifi_signal !== undefined) {
            content += `WiFi: ${d.wifi_signal}dBm<br/>`;
        }

        tooltip.html(content)
            .style('left', (event.pageX + 10) + 'px')
            .style('top', (event.pageY - 10) + 'px');
    }

    hideTooltip() {
        d3.select('.network-tooltip').remove();
    }

    updateSimulation() {
        this.simulation
            .nodes(this.nodes)
            .on('tick', () => this.ticked());

        this.simulation.force('link')
            .links(this.links);

        this.simulation.alpha(1).restart();
    }

    ticked() {
        this.linksGroup.selectAll('line')
            .attr('x1', d => d.source.x)
            .attr('y1', d => d.source.y)
            .attr('x2', d => d.target.x)
            .attr('y2', d => d.target.y);

        this.nodesGroup.selectAll('.node')
            .attr('transform', d => `translate(${d.x},${d.y})`);
    }

    drag() {
        return d3.drag()
            .on('start', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0.3).restart();
                d.fx = d.x;
                d.fy = d.y;
            })
            .on('drag', (event, d) => {
                d.fx = event.x;
                d.fy = event.y;
            })
            .on('end', (event, d) => {
                if (!event.active) this.simulation.alphaTarget(0);
                d.fx = null;
                d.fy = null;
            });
    }

    resize() {
        this.svg
            .attr('width', this.width)
            .attr('height', this.height);

        this.simulation
            .force('center', d3.forceCenter(this.width / 2, this.height / 2))
            .alpha(1)
            .restart();
    }

    // Method to update with bot data from MCP Dashboard
    updateWithBotData(bots) {
        const nodes = [];
        const links = [];

        // Add MCP as central node
        nodes.push({
            id: 'MCP',
            type: 'master',
            label: 'Master Control Program',
            status: 'active'
        });

        // Add bot nodes and WiFi connections
        bots.forEach(bot => {
            nodes.push({
                id: bot.bot_id,
                type: 'bot',
                label: bot.bot_id,
                status: bot.is_active ? 'active' : 'inactive',
                battery_level: bot.battery_level,
                wifi_signal: bot.wifi_signal
            });

            // Add WiFi connection to MCP
            links.push({
                source: bot.bot_id,
                target: 'MCP',
                type: 'wifi',
                active: bot.is_active
            });
        });

        this.updateNetwork({ nodes, links });
    }

    // Method to add ESP-NOW connections based on activity data
    addESPNowConnections(espNowActivity) {
        // Get unique MAC addresses from ESP-NOW activity
        const macAddresses = new Set();
        const connections = new Map();

        espNowActivity.forEach(message => {
            macAddresses.add(message.sender_mac);
            macAddresses.add(message.receiver_mac);

            const key = `${message.sender_mac}->${message.receiver_mac}`;
            if (!connections.has(key)) {
                connections.set(key, {
                    source: message.sender_mac,
                    target: message.receiver_mac,
                    type: 'esp_now',
                    active: true,
                    count: 1
                });
            } else {
                connections.get(key).count++;
            }
        });

        // Add ESP-NOW connections to the network
        const espNowLinks = Array.from(connections.values());
        
        // Only add if we have recent activity
        if (espNowLinks.length > 0) {
            this.links = this.links.concat(espNowLinks);
            this.updateLinks();
            this.updateSimulation();
        }
    }
}

// Initialize network topology when dashboard loads
document.addEventListener('DOMContentLoaded', () => {
    window.networkTopology = new NetworkTopology('topology-svg');
    
    // Update network when bot data changes
    if (window.mcpDashboard) {
        const originalUpdateBotData = window.mcpDashboard.updateBotData;
        window.mcpDashboard.updateBotData = function(bots) {
            originalUpdateBotData.call(this, bots);
            if (window.networkTopology) {
                window.networkTopology.updateWithBotData(bots);
            }
        };
    }
});