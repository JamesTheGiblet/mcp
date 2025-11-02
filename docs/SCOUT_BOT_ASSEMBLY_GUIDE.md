# Scout Bot Assembly Guide - Phase 1 (Everything Except Resistors)

## 🎯 **What We're Building Today**

Your **Advanced Scout Bot** with all components except battery voltage monitoring. The resistors arriving tomorrow will complete the final 5%.

## 📋 **Assembly Checklist**

### **✅ Available Components**

- ✅ ESP32 Type C CH340 Development Board
- ✅ Compact L298N Motor Driver (4-pin)
- ✅ 2x Motors + wheels
- ✅ RGB LED (common cathode)
- ✅ Buzzer
- ✅ RCWL-5016 Motion Sensor
- ✅ TS-YM-115 Sound Sensor  
- ✅ VL53L0X ToF Distance Sensor
- ✅ MPU-9250 9-Axis IMU
- ✅ 2x 3.7V Li-ion batteries + buck converter

### **⏳ Arriving Tomorrow**

- ⏳ 10kΩ and 4.7kΩ resistors (for battery monitoring)

---

## 🔧 **Step-by-Step Assembly**

### **Step 1: Power System Setup**

```txt
Battery Pack (7.4V) → Buck Converter → ESP32 (3.3V) + L298N (5V)

Connections:
- Battery (+) → Buck Converter Input (+)
- Battery (-) → Buck Converter Input (-) 
- Buck 5V Out → L298N VCC
- Buck 3.3V Out → ESP32 VIN (or use USB for now)
- Buck GND → Common ground for all components
```

### **Step 2: L298N Motor Driver**

```txt
L298N Pin | ESP32 Pin | Function
----------|-----------|----------
IN1       | GPIO 23   | Left Motor Control
IN2       | GPIO 22   | Left Motor Control  
IN3       | GPIO 18   | Right Motor Control
IN4       | GPIO 19   | Right Motor Control
VCC       | 5V        | Power from buck converter
GND       | GND       | Common ground

Motor Connections:
- Left Motor → L298N Motor A terminals
- Right Motor → L298N Motor B terminals
```

### **Step 3: RGB LED**

```txt
RGB LED Pin | ESP32 Pin | Function
------------|-----------|----------
Red Anode   | GPIO 2    | Red Channel
Green Anode | GPIO 4    | Green Channel
Blue Anode  | GPIO 5    | Blue Channel
Cathode     | GND       | Common Ground
```

### **Step 4: Audio Alert System**

```txt
Buzzer Pin | ESP32 Pin | Function
-----------|-----------|----------
Positive   | GPIO 32   | Audio Signal
Negative   | GND       | Ground
```

### **Step 5: Motion Detection**

```txt
RCWL-5016 Pin | ESP32 Pin | Function
--------------|-----------|----------
VCC           | 3.3V      | Power
GND           | GND       | Ground
OUT           | GPIO 26   | Motion Signal
```

### **Step 6: Sound Detection**

```txt
TS-YM-115 Pin | ESP32 Pin | Function
--------------|-----------|----------
VCC           | 3.3V      | Power
GND           | GND       | Ground
OUT           | GPIO 17   | Sound Signal
```

### **Step 7: I2C Sensors (VL53L0X + MPU-9250)**

```txt
Both Sensors Share I2C Bus:
Sensor Pin | ESP32 Pin | Function
-----------|-----------|----------
VCC        | 3.3V      | Power (BOTH sensors)
GND        | GND       | Ground (BOTH sensors)
SDA        | GPIO 21   | I2C Data (BOTH sensors)
SCL        | GPIO 25   | I2C Clock (BOTH sensors)

VL53L0X I2C Address: 0x29
MPU-9250 I2C Address: 0x68
```

---

## ⚡ **Power-Up Sequence**

### **1. Initial Power Test**

1. **Connect USB-C** to ESP32 for programming/testing
2. **Power on buck converter** with battery pack
3. **Verify voltages** with multimeter:
   - Buck 5V output → L298N
   - Buck 3.3V output → Sensors
   - All grounds connected

### **2. Component Testing**

Upload firmware and watch serial monitor for:

```txt
✅ WiFi connected! IP address: 192.168.1.xxx
✅ ESP-NOW initialized  
✅ L298N Motor driver initialized
✅ VL53L0X initialized! (if connected)
✅ MPU-9250 initialized! (if connected)
✅ Advanced Scout Bot hardware initialized!
```

### **3. Function Tests**

1. **RGB LED Test**: Should show green on startup
2. **Buzzer Test**: Should play 3-tone startup sequence
3. **Motion Test**: Wave hand near RCWL-5016 → Orange LED
4. **Sound Test**: Make noise → Yellow LED
5. **Motor Test**: Send movement commands via MCP
6. **Distance Test**: Move objects near VL53L0X → Distance readings
7. **IMU Test**: Tilt Scout Bot → Orientation data

---

## 🎨 **Expected RGB LED Behavior**

Once assembled, your Scout Bot RGB LED will show:

```txt
Status                | Color    | Pattern
---------------------|----------|------------------
Startup/Ready        | 🟢 Green | Solid
WiFi Connecting      | 🔵 Blue  | Fast blink
Patrol Mode          | 🔵 Blue  | Slow blink  
Motion Detected      | 🟠 Orange| Solid + beep
Sound Detected       | 🟡 Yellow| Solid + beep
Obstacle Detected    | 🟠 Orange| Fast blink
Emergency/Tilt       | 🔴 Red   | Solid + alarm
Low Battery*         | 🔴 Red   | Fast blink
```

*Low battery detection works tomorrow with resistors

---

## 🎵 **Audio Feedback System**

Your Scout Bot will make these sounds:

```txt
Event              | Sound Pattern
------------------|------------------
Startup           | 1000Hz → 1200Hz → 1400Hz
Motion Detected   | 1000Hz + 1200Hz (2 beeps)
Sound Detected    | 1200Hz (single beep)
Obstacle Warning  | 1200Hz (300ms)
Emergency Stop    | 2000Hz (5 rapid beeps)
Tilt Alert        | 2500Hz alternating pattern
```

---

## 📊 **Real-Time Data Display**

Your Scout Bot will send this data to MCP dashboard:

```json
{
  "bot_id": "Wheelie_Scout_Bot",
  "status": "patrol",
  "sensor_data": {
    "motion_detected": false,
    "sound_detected": false,
    "obstacle_detected": false,
    "distance_mm": 1500,
    "tilt_detected": false,
    "battery_voltage": "monitoring_disabled",
    "imu": {
      "pitch": 0.5,
      "roll": -0.2, 
      "yaw": 45.3,
      "acceleration": 1.02
    }
  },
  "mission_mode": "patrol",
  "activity": "patrolling"
}
```

---

## 🚨 **Troubleshooting Guide**

### **RGB LED Issues**

- **No light**: Check power (3.3V) and common ground
- **Wrong colors**: Verify red→GPIO2, green→GPIO4, blue→GPIO5
- **Dim light**: Check current limiting (may need resistors)

### **Sensor Issues**  

- **I2C errors**: Check SDA→21, SCL→25, and 3.3V power
- **Motion not detected**: RCWL-5016 needs 3-5 second warm-up
- **Sound not detected**: TS-YM-115 may need sensitivity adjustment

### **Motor Issues**

- **No movement**: Verify L298N 5V power and pin connections
- **Wrong direction**: Swap motor wires or check IN1-IN4 pins
- **Weak motors**: Ensure 5V supply can handle motor current

### **Communication Issues**

- **WiFi fails**: Check SSID/password in config.h
- **MCP not found**: Verify server running on 192.168.1.130:8081
- **ESP-NOW errors**: Check for interfering devices

---

## 🎯 **Success Criteria**

Your Scout Bot assembly is **successful** when:

✅ **RGB LED shows status colors**  
✅ **Buzzer plays startup tones**  
✅ **Motion sensor triggers orange LED**  
✅ **Sound sensor triggers yellow LED**  
✅ **Motors respond to commands**  
✅ **Distance sensor reports readings**  
✅ **IMU provides orientation data**  
✅ **WiFi connects and reports to MCP**  
✅ **ESP-NOW mesh network active**  

## 🚀 **What You'll Have Today**

A **95% functional Advanced Scout Bot** with:

- ✅ **Autonomous patrol** with obstacle avoidance
- ✅ **Motion and sound detection**
- ✅ **Real-time sensor monitoring**
- ✅ **RGB status indication**
- ✅ **Audio feedback system**
- ✅ **Wireless control and monitoring**
- ✅ **Emergency safety protocols**

Tomorrow's resistors will add the final **5%**: battery voltage monitoring and low-power protection.

**You're about to have an incredibly capable surveillance robot!** 🤖🎉
