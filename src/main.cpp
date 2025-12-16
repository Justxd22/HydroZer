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
// speed: -255 to 255 (negative = reverse)
void setMotor(int speed, int pinIn1, int pinIn2, int channel) {
    if (speed > 10) { // Forward with deadzone
        digitalWrite(pinIn1, HIGH);
        digitalWrite(pinIn2, LOW);
        ledcWrite(channel, speed);
    } else if (speed < -10) { // Backward with deadzone
        digitalWrite(pinIn1, LOW);
        digitalWrite(pinIn2, HIGH);
        ledcWrite(channel, abs(speed));
    } else { // Stop
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

    // Connected: Solid LED
    digitalWrite(ONBOARD_LED, HIGH);

    // --- Tank Drive Control ---
    // Left Stick Y (ly) controls Left Motors
    // Right Stick Y (ry) controls Right Motors
    // PS3 Y axis: Up is negative (-128), Down is positive (127)
    // We invert it so Up is positive.
    
    int leftY = -Ps3.data.analog.stick.ly; 
    int rightY = -Ps3.data.analog.stick.ry;

    // Map -128..127 to -255..255
    int leftSpeed = map(leftY, -128, 127, -255, 255);
    int rightSpeed = map(rightY, -128, 127, -255, 255);

    setMotor(leftSpeed, IN1, IN2, pwmChannelA);
    setMotor(rightSpeed, IN3, IN4, pwmChannelB);

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
        Serial.printf("L: %d -> %d | R: %d -> %d\n", leftY, leftSpeed, rightY, rightSpeed);
        lastPrint = millis();
    }
    
    delay(10);
}