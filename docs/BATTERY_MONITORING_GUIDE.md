# Scout Bot Battery Monitoring System

## 🔋 **Battery Configuration**

- **Battery Pack**: 2x 3.7V Li-ion batteries in series = **7.4V nominal**
- **Voltage Range**: 6.4V (empty) to 8.4V (fully charged)
- **Buck Converter**: Steps down to 5V (motors) and 3.3V (electronics)

## ⚡ **Voltage Divider Circuit**

### **Components Required**

```txt
Component    | Value  | Tolerance | Purpose
-------------|--------|-----------|------------------
R1 (High)    | 10kΩ   | ±5%      | Voltage dropping resistor
R2 (Low)     | 4.7kΩ  | ±5%      | Reference to ground
Wire         | 22 AWG | -        | Battery connections
```

### **Circuit Diagram**

```txt
Battery Pack (+) ────[R1: 10kΩ]────┬────[R2: 4.7kΩ]──── GND
   7.4V nominal                    │
   6.4V minimum                    └────── ESP32 GPIO 36
   8.4V maximum                           (ADC Input)

Battery Pack (-) ─────────────────────────────────────── GND
```

### **Voltage Calculations**

```txt
Divider Ratio = R2 / (R1 + R2) = 4.7kΩ / (10kΩ + 4.7kΩ) = 0.3197

Battery Voltage | ADC Input | Safe?
----------------|-----------|-------
6.4V (empty)    | 2.05V     | ✅ Yes
7.4V (nominal)  | 2.37V     | ✅ Yes  
8.4V (full)     | 2.69V     | ✅ Yes
```

## 🔌 **Physical Connections**

### **Voltage Divider Wiring**

1. **Battery Pack Positive** → **10kΩ Resistor** → **Junction Point**
2. **Junction Point** → **4.7kΩ Resistor** → **GND**
3. **Junction Point** → **ESP32 GPIO 36**
4. **Battery Pack Negative** → **ESP32 GND**

### **Safety Notes**

- ⚠️ **Never connect battery directly to GPIO 36** - will destroy ESP32!
- ✅ **Always use voltage divider** for safe voltage levels
- 🔧 **Double-check resistor values** before connecting
- 🧪 **Test with multimeter** before powering ESP32

## 📊 **Software Implementation**

### **Battery Status Monitoring**

Your Scout Bot automatically:

```cpp
// Battery voltage thresholds
const float BATTERY_VOLTAGE_MIN = 6.4;  // Emergency stop level
const float BATTERY_VOLTAGE_MAX = 8.4;  // Fully charged level

// Automatic monitoring every loop cycle
scoutState.batteryVoltage = readBatteryVoltage();

// Low battery protection
if (batteryVoltage < BATTERY_VOLTAGE_MIN) {
    scoutState.currentActivity = "low_battery";
    emergencyStop();  // Protect batteries from over-discharge
}
```

### **Battery Level Calculation**

```cpp
// Convert voltage to percentage
float batteryPercent = ((voltage - 6.4) / (8.4 - 6.4)) * 100.0;

// Battery status reporting to MCP
{
  "battery_voltage": 7.4,
  "battery_percent": 50,
  "battery_status": "normal"
}
```

## 🎨 **RGB LED Battery Indicators**

Your Scout Bot shows battery status via RGB LED:

```txt
Battery Level | RGB Color     | Behavior
--------------|---------------|------------------
>70%          | 🟢 Green      | Solid (good)
30-70%        | 🟡 Yellow     | Solid (moderate)  
15-30%        | 🟠 Orange     | Slow blink (low)
<15%          | 🔴 Red        | Fast blink (critical)
<10%          | 🔴 Red        | Emergency stop
```

## 🚨 **Low Battery Protection**

### **Automatic Responses**

When battery drops below safe levels:

1. **30% Remaining**: Warning beep + Orange LED
2. **15% Remaining**: Return to base mode
3. **10% Remaining**: Emergency stop all functions
4. **Critical (<6.4V)**: Complete shutdown to protect batteries

### **Battery Conservation Mode**

```cpp
// Reduced power consumption when battery low
if (batteryPercent < 30) {
    // Reduce motor speeds
    DEFAULT_SPEED = 120;
    TURN_SPEED = 100;
    
    // Increase sensor intervals (save power)
    DISTANCE_SCAN_INTERVAL = 200;  // 200ms instead of 100ms
    IMU_SCAN_INTERVAL = 100;       // 100ms instead of 50ms
    
    // Dim RGB LED
    setRGBColor(255, 165, 0);      // Orange warning
}
```

## 📈 **MCP Dashboard Integration**

Your MCP server receives real-time battery data:

```json
{
  "bot_id": "Wheelie_Scout_Bot",
  "battery_data": {
    "voltage": 7.4,
    "percentage": 50,
    "status": "normal",
    "estimated_runtime": "45 minutes",
    "charging_required": false
  }
}
```

## 🔧 **Assembly Instructions**

### **Step 1: Build Voltage Divider**

1. Solder 10kΩ resistor between battery positive and junction point
2. Solder 4.7kΩ resistor between junction point and ground
3. Connect junction point wire to ESP32 GPIO 36

### **Step 2: Test Circuit**

1. Use multimeter to verify voltage divider output
2. With 7.4V input, should read ~2.37V at junction
3. Confirm safe voltage before connecting to ESP32

### **Step 3: Connect to Scout Bot**

1. Wire battery pack through voltage divider
2. Connect to ESP32 GPIO 36 (ADC input)
3. Upload firmware and test battery readings

## ⚙️ **Advanced Features**

### **Battery Health Monitoring**

- **Voltage trend analysis** over time
- **Charge cycle counting** for battery lifespan
- **Temperature compensation** (if temp sensor added)
- **Predictive maintenance** alerts to MCP

### **Power Management Modes**

```cpp
enum PowerMode {
    POWER_FULL,      // 100% performance
    POWER_BALANCED,  // 80% performance, longer runtime  
    POWER_SAVER,     // 60% performance, maximum runtime
    POWER_CRITICAL   // Emergency operations only
};
```

Your Scout Bot now has **intelligent power management** for extended surveillance missions! 🚀

## 📋 **Shopping List**

```txt
Component         | Quantity | Estimated Cost
------------------|----------|---------------
10kΩ Resistor     | 1        | $0.10
4.7kΩ Resistor    | 1        | $0.10  
22 AWG Wire       | 1 foot   | $0.50
Heat Shrink Tube  | 2 pieces | $0.25
Total             |          | ~$1.00
```

Simple, cheap, and essential for safe battery monitoring! 🔋
