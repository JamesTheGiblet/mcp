# Scout Bot Complete Wiring Guide

## 🎯 **Hardware Overview**

- **ESP32 Type C CH340 Development Board**
- **Compact L298N Motor Driver** (4-pin: IN1, IN2, IN3, IN4)
- **MPU-9250** 9-axis IMU sensor
- **VL53L0X** Time-of-Flight distance sensor
- **RCWL-5016** Motion detection sensor
- **TS-YM-115** Sound detection sensor
- **RGB LED** (common cathode)
- **Buzzer**

## 📋 **Complete Pin Mapping**

### **Motor Control - Compact L298N**

```txt
ESP32 Pin | L298N Pin | Function
----------|-----------|----------
GPIO 23   | IN1       | Left Motor Control
GPIO 22   | IN2       | Left Motor Control  
GPIO 18   | IN3       | Right Motor Control
GPIO 19   | IN4       | Right Motor Control
5V        | VCC       | Power (from buck converter)
GND       | GND       | Ground
```

### **I2C Sensors**

```txt
ESP32 Pin | Sensor Pin | Function
----------|------------|----------
GPIO 21   | SDA        | I2C Data (MPU-9250 & VL53L0X)
GPIO 25   | SCL        | I2C Clock (MPU-9250 & VL53L0X)
3.3V      | VCC        | Power for both sensors
GND       | GND        | Ground for both sensors
```

### **MPU-9250 9-Axis IMU Wiring**

```txt
MPU-9250 Pin | ESP32 Pin | Function
-------------|-----------|----------
VCC          | 3.3V      | Power (3.3V only!)
GND          | GND       | Ground
SDA          | GPIO 21   | I2C Data
SCL          | GPIO 25   | I2C Clock
```

**⚠️ Important MPU-9250 Notes:**

- **Use 3.3V ONLY** - Do not connect to 5V!
- **I2C Address: 0x68** (default)
- **Pull-up resistors** usually built into ESP32 dev board
- **Orientation matters** for proper pitch/roll/yaw readings

### **VL53L0X Time-of-Flight Sensor Wiring**

```txt
VL53L0X Pin | ESP32 Pin | Function
------------|-----------|----------
VCC         | 3.3V      | Power
GND         | GND       | Ground
SDA         | GPIO 21   | I2C Data (shared with MPU)
SCL         | GPIO 25   | I2C Clock (shared with MPU)
```

**⚠️ Important VL53L0X Notes:**

- **I2C Address: 0x29** (default)
- **Range: 30mm to 2000mm** (3cm to 2m)
- **Shares I2C bus** with MPU-9250

### **Other Sensors**

```txt
Component    | ESP32 Pin | Function
-------------|-----------|----------
RCWL-5016    | GPIO 26   | Motion Detection (digital)
TS-YM-115    | GPIO 17   | Sound Detection (digital)
Buzzer       | GPIO 32   | Audio Alerts
```

### **RGB LED Wiring**

```txt
RGB LED Pin | ESP32 Pin | Function
------------|-----------|----------
Red         | GPIO 2    | Red Channel (PWM)
Green       | GPIO 4    | Green Channel (PWM)
Blue        | GPIO 5    | Blue Channel (PWM)
Common GND  | GND       | Ground (common cathode)
```

## 🔌 **Power Distribution**

### **Main Power System**

```txt
Battery Pack (2x 3.7V in series = 7.4V)
    ↓
Buck Converter
    ├── 5V → L298N Motor Driver
    └── 3.3V → ESP32, MPU-9250, VL53L0X
```

### **Power Connections**

```txt
Power Rail  | Components
------------|------------------
7.4V Battery| Buck Converter Input
5V Output   | L298N VCC, Motors
3.3V Output | ESP32, MPU-9250, VL53L0X
GND Common  | All components share common ground
```

## 🎨 **RGB LED Status Colors**

Your Scout Bot will display different colors based on its status:

```txt
Color          | Status
---------------|------------------
🟢 Green       | Ready/Initialized
🔵 Blue        | Patrolling
🔴 Red         | Alert/Error
🟠 Orange      | Motion Detected
🟡 Yellow      | Sound Detected
🟣 Purple      | Stealth Mode
⚪ White       | Unknown Status
⚫ Off         | Blinking pattern
```

## 🔧 **Sensor Integration Status**

### **MPU-9250 Capabilities**

- ✅ **3-axis Accelerometer** - Detects tilt and impact
- ✅ **3-axis Gyroscope** - Measures rotation
- ✅ **3-axis Magnetometer** - Compass heading
- ✅ **Temperature sensor** - Environment monitoring
- ✅ **Digital Motion Processor** - Advanced algorithms

### **Detection Thresholds (Configured)**

```txt
Sensor      | Threshold        | Response
------------|------------------|------------------
Tilt        | ±30°            | Emergency stop + 5 beeps
Impact      | >2G acceleration| Emergency stop + alarm
Rotation    | >90°/s          | Stability warning
Distance    | <200mm          | Obstacle avoidance
Close       | <100mm          | Emergency backup
```

## 📡 **I2C Bus Configuration**

Your I2C bus is configured for **400kHz** speed with two devices:

```txt
Address | Device      | Function
--------|-------------|------------------
0x68    | MPU-9250    | 9-axis IMU sensor
0x29    | VL53L0X     | Distance sensor
```

## 🎯 **Next Steps for Assembly**

1. **Connect Power System** - Buck converter + battery pack
2. **Wire L298N Motor Driver** - 4 GPIO pins + power
3. **Connect I2C Sensors** - MPU-9250 + VL53L0X on pins 21/25
4. **Install RGB LED** - Pins 2, 4, 5 + common ground
5. **Add Other Sensors** - RCWL-5016, TS-YM-115, buzzer
6. **Test Each Subsystem** - Verify connections before full assembly

## ⚠️ **Critical Safety Notes**

- **MPU-9250**: Use 3.3V only! 5V will damage the sensor
- **I2C Pull-ups**: ESP32 dev board usually has built-in pull-ups
- **Power isolation**: Keep motor power (5V) separate from sensor power (3.3V)
- **Common ground**: All components must share the same ground reference
- **Motor current**: Ensure buck converter can handle motor load + electronics

Your Scout Bot is now ready for **professional-grade surveillance** with advanced motion tracking! 🚀
