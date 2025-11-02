# Advanced Scout Bot Hardware Configuration

## 🎯 **Complete Hardware Specifications**

Your **Wheelie Scout Bot** is now configured for your **exact hardware setup**:

### ⚡ **Power System**

- **2x 3.7V Li-ion batteries in series** (7.4V total)
- **Buck converter** for voltage regulation to 5V/3.3V
- **Battery voltage monitoring** via ADC pin 36
- **Low battery protection** with emergency stop at <20%

### 🚗 **Motor Control**

- **L298N Dual H-Bridge Motor Driver Board**
  - ENA (GPIO 18) - Left motor PWM speed
  - IN1 (GPIO 19) + IN2 (GPIO 21) - Left motor direction
  - ENB (GPIO 22) - Right motor PWM speed  
  - IN3 (GPIO 23) + IN4 (GPIO 25) - Right motor direction
- **2 motors + front caster wheel** configuration
- **Speed ranges**: 80-220 PWM, stealth mode 100, patrol 180

### 📡 **Advanced Sensor Array**

#### **Motion & Interaction Detection**

- **RCWL-5016** (GPIO 26) - Microwave motion sensor
- **TS-YM-115** (GPIO 27 + analog 35) - Sound detection sensor

#### **Precision Distance Measurement**

- **VL53L0X Time-of-Flight Sensor** (I2C 0x29)
  - Range: 30mm to 2000mm
  - Accuracy: ±3% at 2m
  - Obstacle detection at 200mm
  - Close approach warning at 100mm

#### **9-Axis Motion Monitoring**

- **MPU-9250** (I2C 0x68) - 9-axis IMU
  - **3-axis accelerometer** (±2g, ±4g, ±8g, ±16g)
  - **3-axis gyroscope** (±250, ±500, ±1000, ±2000 °/s)  
  - **3-axis magnetometer** (±4800 μT)
  - **Tilt detection** at ±30°
  - **Impact detection** at >2g acceleration
  - **Orientation tracking** (pitch, roll, yaw)

#### **I2C Configuration**

- **SDA**: GPIO 21
- **SCL**: GPIO 22  
- **Clock**: 400kHz for optimal performance

### 🔔 **Status & Alerts**

- **Status LED** (GPIO 2) - Visual indication with blinking patterns
- **Buzzer** (GPIO 32) - Audio alerts with different tones for different events

## 🎯 **Mission Capabilities**

### **Mission Modes**

1. **🔍 PATROL** - Autonomous 45-second patrol cycles with random turns
2. **🥷 STEALTH** - Silent reconnaissance at 100 PWM speed
3. **🚨 ALERT** - High-alert rapid response mode
4. **🔎 INVESTIGATE** - Targeted investigation of detected activity
5. **⏸️ STANDBY** - Monitor-only mode, no movement

### **Detection & Response Matrix**

| Sensor | Trigger | Response | Alert Level |
|--------|---------|----------|-------------|
| **Motion** | RCWL-5016 HIGH | Pause → Sound alert → Investigate | 2 (Caution) |
| **Sound** | TS-YM-115 HIGH | Brief alert → Continue patrol | 1 (Low) |
| **Obstacle** | <200mm | Stop → Alert → Avoid maneuver | 2 (Caution) |
| **Close approach** | <100mm | Emergency back + turn | 3 (High) |
| **Tilt** | >30° pitch/roll | Emergency stop → 5 beeps | 3 (High) |
| **Impact** | >2g acceleration | Emergency stop → Alarm | 3 (High) |
| **Low battery** | <20% | Emergency stop → Warning | Emergency |

### **Communication Protocol**

#### **ESP-NOW Commands**

- **Movement**: `forward`, `backward`, `left`, `right`, `stop`
- **Missions**: `patrol`, `stealth`, `alert`, `investigate`, `standby`
- **Special**: `emergency_stop`, `sound_test`

#### **Status Data to MCP**

```json
{
  "bot_type": "scout_surveillance_advanced",
  "sensor_data": {
    "motion_detected": true/false,
    "sound_detected": true/false,
    "obstacle_detected": true/false,
    "tilt_detected": true/false,
    "impact_detected": true/false,
    "distance_mm": 1500,
    "distance_cm": 150.0,
    "mission_mode": 1,
    "alert_level": 2,
    "battery_voltage": 7.4,
    "imu": {
      "accel_x": 0.02, "accel_y": 0.01, "accel_z": 1.0,
      "gyro_x": 0.5, "gyro_y": -0.2, "gyro_z": 0.1,
      "pitch": 2.5, "roll": -1.2, "yaw": 45.8,
      "temperature": 24.5
    }
  }
}
```

## ⚡ **Performance Optimized**

- **Build Success**: 81.2% Flash, 16.0% RAM usage
- **Sensor Update Rates**:
  - Distance: 100ms (10Hz)
  - IMU: 50ms (20Hz)  
  - Motion: 500ms
  - Sound: 200ms
- **I2C Speed**: 400kHz for responsive sensor readings
- **Memory Efficient**: 512-byte status messages to peers

## 🎮 **Audio Feedback System**

- **Welcome**: 3-tone startup sequence (1000Hz → 1200Hz → 1400Hz)
- **Mission Changes**: Unique tones per mode
- **Motion Alert**: 1000Hz + 1200Hz sequence
- **Obstacle**: 1200Hz for 300ms
- **Close Approach**: Dual 1500Hz beeps
- **Tilt Alert**: 5 × 2000Hz beeps
- **Impact Alert**: Alternating 2500Hz + 2000Hz

Your Advanced Scout Bot is now **hardware-ready** with professional-grade sensing capabilities! 🚀

## 📋 **Pin Summary**

```txt
ESP32-38 Pin | Function | Component
-------------|----------|----------
GPIO 18 | ENA (Left PWM) | L298N
GPIO 19 | IN1 (Left Dir) | L298N  
GPIO 21 | IN2 (Left Dir) + SDA | L298N + I2C
GPIO 22 | ENB (Right PWM) + SCL | L298N + I2C
GPIO 23 | IN3 (Right Dir) | L298N
GPIO 25 | IN4 (Right Dir) | L298N
GPIO 26 | Motion Detection | RCWL-5016
GPIO 27 | Sound Detection | TS-YM-115
GPIO 32 | Buzzer | Buzzer
GPIO 2  | Status LED | LED
GPIO 35 | Sound Analog | TS-YM-115
GPIO 36 | Battery Monitor | Voltage Divider
I2C 0x29 | Distance Sensor | VL53L0X
I2C 0x68 | 9-Axis IMU | MPU-9250
```
