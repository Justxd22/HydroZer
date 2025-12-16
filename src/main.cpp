#include <Arduino.h>
#include <Ps3Controller.h>

// --- Pin Definitions ---
// L298N Control Pins
const int IN1 = 26; // Left Motor Direction 1
const int IN2 = 27; // Left Motor Direction 2
const int ENA = 25; // Left Motor Speed (PWM)

const int IN3 = 14; // Right Motor Direction 1
const int IN4 = 12; // Right Motor Direction 2
const int ENB = 13; // Right Motor Speed (PWM)

const int ONBOARD_LED = 32;

// PWM Properties
const int freq = 30000;
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int resolution = 8; // 0-255 resolution

void setup() {
    Serial.begin(115200);

    // Setup Motor Pins
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(ONBOARD_LED, OUTPUT);

    // Setup PWM
    ledcSetup(pwmChannelA, freq, resolution);
    ledcSetup(pwmChannelB, freq, resolution);
    ledcAttachPin(ENA, pwmChannelA);
    ledcAttachPin(ENB, pwmChannelB);

    // Initialize PS3
    // REPLACE WITH YOUR CONTROLLER'S MASTER MAC OR "01:02:03:04:05:06"
    Ps3.begin("9C:B6:D0:DD:9D:10"); 

    Serial.println("Ready. Waiting for PS3 Controller...");
}

// Helper to control a single motor channel
void setMotor(int speed, int pinIn1, int pinIn2, int channel) {
    // constrain speed to valid PWM range
    speed = constrain(speed, -255, 255);

    if (speed > 20) { // Forward (positive speed)
        digitalWrite(pinIn1, HIGH);
        digitalWrite(pinIn2, LOW);
        ledcWrite(channel, speed);
    } else if (speed < -20) { // Backward (negative speed)
        digitalWrite(pinIn1, LOW);
        digitalWrite(pinIn2, HIGH);
        ledcWrite(channel, abs(speed));
    } else { // Stop (deadzone)
        digitalWrite(pinIn1, LOW);
        digitalWrite(pinIn2, LOW);
        ledcWrite(channel, 0);
    }
}

void loop() {
    if (!Ps3.isConnected()) {
        // Blink LED to indicate disconnected
        digitalWrite(ONBOARD_LED, (millis() / 500) % 2);
        return;
    }
    digitalWrite(ONBOARD_LED, HIGH);

    // --- Arcade Drive Logic (Left Stick Only) ---
    // LY: Up = -128, Down = 127. We invert so Up is positive.
    // LX: Left = -128, Right = 127.
    
    int throttle = -Ps3.data.analog.stick.ly; 
    int steering = Ps3.data.analog.stick.lx;  

    // Deadzone adjustment for stick drift
    if (abs(throttle) < 10) throttle = 0;
    if (abs(steering) < 10) steering = 0;

    // Mixing algorithm
    // Move Forward: Both positive
    // Turn Right: Left increases, Right decreases
    int leftMotorSpeed = throttle + steering;
    int rightMotorSpeed = throttle - steering;

    // Map the stick values (-128 to 127) roughly to PWM (-255 to 255)
    // We multiply by 2 to get close to full PWM range
    leftMotorSpeed = leftMotorSpeed * 2;
    rightMotorSpeed = rightMotorSpeed * 2;

    // Apply to motors
    setMotor(leftMotorSpeed, IN1, IN2, pwmChannelA);
    setMotor(rightMotorSpeed, IN3, IN4, pwmChannelB);

    // Debugging
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
        Serial.printf("Stick Y: %d, X: %d || L_Speed: %d, R_Speed: %d\n", 
                      throttle, steering, leftMotorSpeed, rightMotorSpeed);
        lastPrint = millis();
    }
    
    delay(10);
}