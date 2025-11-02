/*
 * ESP32 Scout Bot - INCREMENTAL TEST VERSION
 * Phase 6: RGB LED + Buzzer + Motion + Sound + ToF Distance Sensor Testing
 * 
 * This version tests RGB LED + Buzzer + RCWL-5016 Motion + TS-YM-115 Sound + VL53L0X ToF
 * RGB LED cycles colors, Buzzer beeps, Motion and Sound detection change behavior
 * ToF sensor measures distances and detects obstacles
 */

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "config.h"

// Global variables for testing
VL53L0X tofSensor;
bool tofSensorReady = false;
unsigned long lastDistanceCheck = 0;
int currentDistance = 0;
bool obstacleDetected = false;
bool isObstacleMode = false;

unsigned long lastColorChange = 0;
int currentColor = 0;
bool motionDetected = false;
unsigned long lastMotionTime = 0;
bool isMotionMode = false;
bool soundDetected = false;
unsigned long lastSoundTime = 0;
bool isSoundMode = false;

// Sound filtering variables
bool buzzerIsActive = false;
unsigned long buzzerStartTime = 0;
unsigned long buzzerQuietTime = 500; // Wait 500ms after buzzer stops before listening for external sounds

// RGB LED Functions
void setRGBColor(int red, int green, int blue) {
    analogWrite(LED_RED_PIN, red);
    analogWrite(LED_GREEN_PIN, green);
    analogWrite(LED_BLUE_PIN, blue);
}

// Buzzer Functions
void buzzerBeep(int frequency, int duration) {
    buzzerIsActive = true;
    buzzerStartTime = millis();
    
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    noTone(BUZZER_PIN);
    
    buzzerIsActive = false;
    buzzerStartTime = millis(); // Start quiet period
}

void buzzerDoubleBeep() {
    buzzerBeep(1000, 100);  // High beep
    delay(50);
    buzzerBeep(800, 100);   // Lower beep
}

// Motion Sensor Functions
void checkMotionSensor() {
    bool currentMotion = digitalRead(RCWL_5016_PIN);
    
    if (currentMotion && !motionDetected) {
        // Motion just detected
        motionDetected = true;
        lastMotionTime = millis();
        isMotionMode = true;
        
        Serial.println("🚨 MOTION DETECTED! Switching to Alert Mode (Visual Only)");
        
        // Visual alert only - no buzzer sounds
        // Flash red for alert
        setRGBColor(255, 0, 0);
        delay(500);
        setRGBColor(0, 0, 0);
        delay(200);
        setRGBColor(255, 0, 0);
        delay(500);
        setRGBColor(0, 0, 0);
        
    } else if (!currentMotion && motionDetected) {
        // Motion stopped
        motionDetected = false;
        Serial.println("✅ Motion cleared - Returning to normal mode (Silent)");
        
        // After 5 seconds of no motion, exit motion mode
        if (millis() - lastMotionTime > 5000) {
            isMotionMode = false;
            Serial.println("🔄 Returning to color cycle mode");
        }
    }
}

// ToF Distance Sensor Functions
void scanI2CDevices() {
    Serial.println("🔍 Scanning I2C bus for devices...");
    int deviceCount = 0;
    
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.printf("✅ I2C device found at address 0x%02X\n", address);
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("❌ No I2C devices found! Check wiring:");
        Serial.println("   VL53L0X VIN → ESP32 3.3V");
        Serial.println("   VL53L0X GND → ESP32 GND");
        Serial.println("   VL53L0X SDA → ESP32 GPIO 21");
        Serial.println("   VL53L0X SCL → ESP32 GPIO 25");
    } else {
        Serial.printf("🔍 Found %d I2C device(s)\n", deviceCount);
    }
    Serial.println();
}

void initializeToFSensor() {
    Serial.println("🔧 Initializing I2C and VL53L0X ToF Distance Sensor...");
    
    // Initialize I2C with explicit pins
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000); // 100kHz for more reliable communication
    delay(100);
    
    // Scan for I2C devices first
    scanI2CDevices();
    
    Serial.println("🔧 Attempting VL53L0X initialization...");
    tofSensor.setTimeout(500);
    
    if (!tofSensor.init()) {
        Serial.println("❌ Failed to detect and initialize VL53L0X sensor!");
        Serial.println("💡 This is OK - ToF sensor might not be connected yet");
        Serial.println("   Other sensors will continue working normally");
        tofSensorReady = false;
    } else {
        Serial.println("✅ VL53L0X ToF sensor initialized successfully!");
        Serial.println("📏 Starting continuous distance measurements...");
        tofSensor.setMeasurementTimingBudget(33000); // 33ms for faster readings
        tofSensor.startContinuous(100); // Take a reading every 100ms
        tofSensorReady = true;
    }
}

void checkToFSensor() {
    if (!tofSensorReady || millis() - lastDistanceCheck < DISTANCE_SCAN_INTERVAL) {
        return;
    }
    
    lastDistanceCheck = millis();
    
    uint16_t distance = tofSensor.readRangeContinuousMillimeters();
    
    if (tofSensor.timeoutOccurred()) {
        Serial.println("⚠️  ToF sensor timeout - sensor may not be connected");
        return;
    }
    
    // Filter out obviously bad readings
    if (distance > 8000) {
        return; // Ignore readings > 8 meters
    }
    
    currentDistance = distance;
    
    // Check for obstacles
    bool currentObstacle = (distance < OBSTACLE_DISTANCE_MM && distance > 30); // Ignore very close readings (<3cm)
    
    if (currentObstacle && !obstacleDetected) {
        // Obstacle just detected
        obstacleDetected = true;
        isObstacleMode = true;
        
        Serial.printf("🚧 OBSTACLE DETECTED! Distance: %dmm (%dcm)\n", distance, distance/10);
        
        // Obstacle detection sound alert
        buzzerBeep(1500, 150);  // High pitch warning
        buzzerBeep(1200, 150);  // Medium pitch
        buzzerBeep(1500, 150);  // High pitch warning
        
        // Orange flash for obstacle
        setRGBColor(255, 165, 0);  // Orange
        delay(300);
        setRGBColor(0, 0, 0);
        delay(100);
        setRGBColor(255, 165, 0);  // Orange
        delay(300);
        setRGBColor(0, 0, 0);
        
    } else if (!currentObstacle && obstacleDetected) {
        // Obstacle cleared
        obstacleDetected = false;
        isObstacleMode = false;
        Serial.printf("✅ Obstacle cleared - Distance: %dmm (%dcm)\n", distance, distance/10);
        
        // Clear sound
        buzzerBeep(800, 200);
        buzzerBeep(1000, 200);
    }
    
    // Regular distance reporting (every 3 seconds)
    static unsigned long lastDistanceReport = 0;
    if (millis() - lastDistanceReport > 3000) {
        Serial.printf("📏 ToF Distance: %dmm (%d.%dcm)\n", distance, distance/10, (distance%10));
        lastDistanceReport = millis();
    }
}
void checkSoundSensor() {
    bool currentSound = digitalRead(SOUND_SENSOR_PIN);
    
    // Check if we should ignore sounds (buzzer active or in quiet period)
    bool shouldIgnoreSound = buzzerIsActive || (millis() - buzzerStartTime < buzzerQuietTime);
    
    if (currentSound) {
        if (shouldIgnoreSound) {
            // Debug: Show that we detected our own sound
            static unsigned long lastSelfSoundReport = 0;
            if (millis() - lastSelfSoundReport > 1000) { // Report max once per second
                Serial.println("🔇 Sound detected but ignoring (own buzzer sound)");
                lastSelfSoundReport = millis();
            }
            return; // Ignore our own sounds
        }
        
        if (!soundDetected) {
            // External sound detected!
            soundDetected = true;
            lastSoundTime = millis();
            isSoundMode = true;
            
            Serial.println("🔊 EXTERNAL SOUND DETECTED! Activating Sound Response Mode");
            
            // Sound detection response - musical pattern
            buzzerBeep(523, 150);  // C note
            buzzerBeep(659, 150);  // E note
            buzzerBeep(784, 150);  // G note
            buzzerBeep(1047, 200); // High C note
            
            // Bright white flash for sound detection
            setRGBColor(255, 255, 255);  // White
            delay(300);
            setRGBColor(255, 255, 0);    // Yellow
            delay(300);
            setRGBColor(255, 255, 255);  // White
            delay(300);
        }
        
    } else if (!currentSound && soundDetected) {
        // External sound stopped
        soundDetected = false;
        Serial.println("✅ External sound cleared - Returning to normal mode");
        
        // Descending clear sound
        buzzerBeep(1047, 100);  // High C
        buzzerBeep(784, 100);   // G
        buzzerBeep(659, 100);   // E
        buzzerBeep(523, 150);   // C
        
        // After 3 seconds of no sound, exit sound mode
        if (millis() - lastSoundTime > 3000) {
            isSoundMode = false;
            Serial.println("🔄 Returning to color cycle mode");
        }
    }
}

void testRGBWithBuzzerMotionSoundAndToF() {
    // Check sensors in priority order
    checkMotionSensor();
    checkSoundSensor();
    checkToFSensor();
    
    // Motion has highest priority
    if (isMotionMode) {
        // Fast red/blue alternating during motion mode
        static unsigned long lastFlash = 0;
        static bool flashState = false;
        
        if (millis() - lastFlash > 300) {
            if (flashState) {
                setRGBColor(255, 0, 0);  // Red
                Serial.println("🚨 ALERT: RED Flash (Silent)");
            } else {
                setRGBColor(0, 0, 255);  // Blue
                Serial.println("🚨 ALERT: BLUE Flash (Silent)");
            }
            flashState = !flashState;
            lastFlash = millis();
            
            // No more alert beeps - visual only
        }
        return;  // Skip normal color cycle
    }
    
    // Obstacle mode has second priority
    if (isObstacleMode) {
        // Pulsing orange during obstacle mode
        static unsigned long lastPulse = 0;
        static bool pulseState = false;
        
        if (millis() - lastPulse > 400) {
            if (pulseState) {
                setRGBColor(255, 165, 0);  // Orange
                Serial.printf("🚧 OBSTACLE MODE: Orange Pulse - %dmm\n", currentDistance);
                buzzerBeep(1200, 80);      // Warning tone
            } else {
                setRGBColor(100, 50, 0);   // Dim orange
                Serial.printf("🚧 OBSTACLE MODE: Dim Orange - %dmm\n", currentDistance);
            }
            pulseState = !pulseState;
            lastPulse = millis();
        }
        return;  // Skip normal color cycle
    }
    
    // Sound mode has third priority
    if (isSoundMode) {
        // Pulsing white/yellow during sound mode
        static unsigned long lastPulse = 0;
        static bool pulseState = false;
        
        if (millis() - lastPulse > 500) {
            if (pulseState) {
                setRGBColor(255, 255, 255);  // Bright white
                Serial.println("🔊 SOUND MODE: WHITE Pulse");
                buzzerBeep(1000, 100);       // Musical tone
            } else {
                setRGBColor(255, 255, 0);    // Yellow
                Serial.println("🔊 SOUND MODE: YELLOW Pulse");
                buzzerBeep(1200, 100);       // Higher musical tone
            }
            pulseState = !pulseState;
            lastPulse = millis();
        }
        return;  // Skip normal color cycle
    }
    
    // Normal color cycle mode
    // Cycle through different colors every 3 seconds (longer for buzzer demo)
    if (millis() - lastColorChange > 3000) {
        switch (currentColor) {
            case 0:
                setRGBColor(255, 0, 0);     // Red
                Serial.println("🔴 RGB LED: RED + 🔊 Single Beep");
                buzzerBeep(1000, 200);      // High beep for red
                break;
            case 1:
                setRGBColor(0, 255, 0);     // Green
                Serial.println("🟢 RGB LED: GREEN + 🔊 Double Beep");
                buzzerDoubleBeep();         // Double beep for green
                break;
            case 2:
                setRGBColor(0, 0, 255);     // Blue
                Serial.println("🔵 RGB LED: BLUE + 🔊 Low Beep");
                buzzerBeep(500, 300);       // Low beep for blue
                break;
            case 3:
                setRGBColor(255, 255, 0);   // Yellow
                Serial.println("🟡 RGB LED: YELLOW + 🔊 Quick Beeps");
                buzzerBeep(1500, 100);
                delay(100);
                buzzerBeep(1500, 100);
                delay(100);
                buzzerBeep(1500, 100);      // Triple quick beeps
                break;
            case 4:
                setRGBColor(255, 0, 255);   // Magenta
                Serial.println("🟣 RGB LED: MAGENTA + 🔊 Ascending Tones");
                buzzerBeep(800, 150);
                buzzerBeep(1000, 150);
                buzzerBeep(1200, 150);      // Ascending tones
                break;
            case 5:
                setRGBColor(0, 255, 255);   // Cyan
                Serial.println("🔷 RGB LED: CYAN + 🔊 Descending Tones");
                buzzerBeep(1200, 150);
                buzzerBeep(1000, 150);
                buzzerBeep(800, 150);       // Descending tones
                break;
            case 6:
                setRGBColor(255, 255, 255); // White
                Serial.println("⚪ RGB LED: WHITE + 🔊 Happy Melody");
                buzzerBeep(523, 200);  // C
                buzzerBeep(659, 200);  // E
                buzzerBeep(784, 200);  // G
                break;
            case 7:
                setRGBColor(0, 0, 0);       // Off
                Serial.println("⚫ RGB LED: OFF + 🔊 Silent");
                // No buzzer for "off" state
                break;
        }
        
        currentColor = (currentColor + 1) % 8;
        lastColorChange = millis();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("===============================================");
    Serial.println("🔬 ESP32 Scout Bot - RGB+Buzzer+Motion+Sound+ToF Test");
    Serial.println("===============================================");
    Serial.println();
    Serial.println("Testing components on pins:");
    Serial.printf("  Red LED:   GPIO %d\n", LED_RED_PIN);
    Serial.printf("  Green LED: GPIO %d\n", LED_GREEN_PIN);
    Serial.printf("  Blue LED:  GPIO %d\n", LED_BLUE_PIN);
    Serial.printf("  Buzzer:    GPIO %d\n", BUZZER_PIN);
    Serial.printf("  Motion:    GPIO %d (RCWL-5016)\n", RCWL_5016_PIN);
    Serial.printf("  Sound:     GPIO %d (TS-YM-115)\n", SOUND_SENSOR_PIN);
    Serial.printf("  I2C SDA:   GPIO %d\n", I2C_SDA);
    Serial.printf("  I2C SCL:   GPIO %d\n", I2C_SCL);
    Serial.println("  ToF Sensor: VL53L0X (I2C Address: 0x29)");
    Serial.println();
    
    // Initialize RGB LED pins
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    
    // Initialize Buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Initialize Motion Sensor pin
    pinMode(RCWL_5016_PIN, INPUT);
    
    // Initialize Sound Sensor pin
    pinMode(SOUND_SENSOR_PIN, INPUT);
    
    // Initialize ToF Distance Sensor
    initializeToFSensor();
    
    // Start with green (ready) and welcome beep
    setRGBColor(0, 255, 0);
    Serial.println("✅ Components initialized - Starting test sequence...");
    buzzerBeep(1000, 300);  // Welcome beep
    Serial.println();
    Serial.println("Normal Mode: Color cycle every 3 seconds");
    Serial.println("🔴 Red+Beep → 🟢 Green+Double → 🔵 Blue+Low → 🟡 Yellow+Triple → 🟣 Magenta+Rising → 🔷 Cyan+Falling → ⚪ White+Melody → ⚫ Off+Silent");
    Serial.println();
    Serial.println("🚨 Motion Detection: Wave hand near RCWL-5016 sensor for alert mode!");
    Serial.println("   Alert Mode: Fast Red/Blue flashing (SILENT - no buzzer)");
    Serial.println();
    Serial.println("� Obstacle Detection: Place object <20cm from VL53L0X sensor!");
    Serial.println("   Obstacle Mode: Orange pulsing + warning beeps");
    Serial.println();
    Serial.println("�🔊 Sound Detection: Clap or make noise near TS-YM-115 sensor!");
    Serial.println("   Sound Mode: White/Yellow pulsing + musical tones");
    Serial.println("   🔇 Smart filtering: Ignores own buzzer sounds, only responds to external sounds");
    Serial.println();
    Serial.println("📏 Distance Monitoring: Continuous distance measurement every 100ms");
    Serial.println("   Reports distance every 2 seconds");
    Serial.println();
}

void loop() {
    testRGBWithBuzzerMotionSoundAndToF();
    delay(100); // Small delay for smooth operation
}