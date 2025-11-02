# Phase 5 Preparation - Motor Driver Integration

## 🎯 **Tomorrow's Mission: Add Mobility to the Scout Bot!**

### **Current State**

✅ All sensors working perfectly together  
✅ Smart priority system operational  
✅ ToF distance sensor providing real-time data  
✅ Silent motion detection with visual alerts  
✅ Smart sound filtering preventing feedback loops  

### **Phase 5 Goals**

🎯 Add L298N motor driver functionality  
🎯 Implement basic movement controls (forward, backward, left, right, stop)  
🎯 Maintain all existing sensor functionality  
🎯 Add safety features and speed controls  
🎯 Test autonomous obstacle avoidance using ToF sensor  

### **Hardware Preparation Checklist**

- [ ] Connect L298N IN1 → GPIO 22 (Left Motor Direction A)
- [ ] Connect L298N IN2 → GPIO 23 (Left Motor Direction B)  
- [ ] Connect L298N IN3 → GPIO 19 (Right Motor Direction A)
- [ ] Connect L298N IN4 → GPIO 18 (Right Motor Direction B)
- [ ] Connect L298N VCC → 7.4V Battery (through buck converter)
- [ ] Connect L298N GND → ESP32 GND
- [ ] Connect OUT1/OUT2 → Left Motor
- [ ] Connect OUT3/OUT4 → Right Motor

### **Code Features to Implement**

1. **Motor Control Functions**:

   ```cpp
   void moveForward(int speed, int duration);
   void moveBackward(int speed, int duration);
   void turnLeft(int speed, int duration);
   void turnRight(int speed, int duration);
   void stopMotors();
   ```

2. **Safety Features**:
   - Speed limiting (80-220 PWM range)
   - Movement timeouts to prevent runaway
   - Emergency stop functionality
   - Obstacle detection integration

3. **Autonomous Behaviors**:
   - Stop when obstacle detected (<20cm)
   - Back up and turn when blocked
   - Random patrol patterns
   - Motion-triggered investigation mode

### **Testing Sequence**

1. **Phase 5a**: Basic motor control (forward/backward/stop)
2. **Phase 5b**: Turning and steering control
3. **Phase 5c**: Speed control and safety limits
4. **Phase 5d**: ToF obstacle avoidance integration
5. **Phase 5e**: Motion-triggered autonomous behavior

### **Expected Challenges**

- Power management with motors drawing current
- Ensuring sensors continue working during movement
- Tuning movement speeds and durations
- Balancing autonomous vs manual control

### **Success Criteria**

✅ Motors respond to all direction commands  
✅ All existing sensors remain functional  
✅ ToF sensor prevents collisions automatically  
✅ Motion detection triggers investigation behavior  
✅ Safe operation with emergency stop capability  

## Ready to make the Scout Bot MOBILE! 🤖🚗
