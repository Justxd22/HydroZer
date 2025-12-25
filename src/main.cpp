#include <Arduino.h>
#include <Ps3Controller.h>

// --- Pin Definitions ---

// --- DRIVING MOTORS (L298N #1) ---
const int IN1 = 26; // Left Motor Dir 1
const int IN2 = 27; // Left Motor Dir 2
const int ENA = 25; // Left Motor Speed (PWM)

const int IN3 = 14; // Right Motor Dir 1
const int IN4 = 12; // Right Motor Dir 2
const int ENB = 13; // Right Motor Speed (PWM)

// --- ARM MOTORS (L298N #2 & #3) ---
// Arm Motor 1 (Controlled by R2/L2)
const int ARM1_IN1 = 16;
const int ARM1_IN2 = 17;

// Arm Motor 2 (Controlled by R1/L1)
const int ARM2_IN1 = 18;
const int ARM2_IN2 = 19;

// Arm Motor 3 (Controlled by Triangle/Cross & Circle/Square)
const int ARM3_IN1 = 22;
const int ARM3_IN2 = 23;

// Arm Motor 4 (Controlled by Right Joystick)
const int ARM4_IN1 = 32; 
const int ARM4_IN2 = 33; 

// --- EXTRAS ---
const int ONBOARD_LED = 15; 
const int BUZZER_PIN = 4; // Active Buzzer

// PWM Properties (Driving Motors Only)
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
    pinMode(BUZZER_PIN, OUTPUT);

    // Setup PWM
    ledcSetup(pwmChannelA, freq, resolution);
    ledcSetup(pwmChannelB, freq, resolution);
    ledcAttachPin(ENA, pwmChannelA);
    ledcAttachPin(ENB, pwmChannelB);

    // --- Setup Arm Pins ---
    pinMode(ARM1_IN1, OUTPUT);
    pinMode(ARM1_IN2, OUTPUT);
    
    pinMode(ARM2_IN1, OUTPUT);
    pinMode(ARM2_IN2, OUTPUT);

    pinMode(ARM3_IN1, OUTPUT);
    pinMode(ARM3_IN2, OUTPUT);

    pinMode(ARM4_IN1, OUTPUT);
    pinMode(ARM4_IN2, OUTPUT);

    // Startup Beep Sequence
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);

    // Initialize PS3
    Ps3.begin("9C:B6:D0:DD:9D:10"); 

    Serial.println("Ready. Waiting for PS3 Controller...");
}

// Helper: Drive Wheels with Speed Control
void setDriveMotor(int speed, int pinIn1, int pinIn2, int channel) {
    speed = constrain(speed, -255, 255);
    if (speed > 20) {
        digitalWrite(pinIn1, HIGH);
        digitalWrite(pinIn2, LOW);
        ledcWrite(channel, speed);
    } else if (speed < -20) {
        digitalWrite(pinIn1, LOW);
        digitalWrite(pinIn2, HIGH);
        ledcWrite(channel, abs(speed));
    } else {
        digitalWrite(pinIn1, LOW);
        digitalWrite(pinIn2, LOW);
        ledcWrite(channel, 0);
    }
}

// Helper: Arm Motors (Full Speed)
// State: 1 = Forward, -1 = Backward, 0 = Stop
void setArmMotor(int state, int pin1, int pin2) {
    if (state == 1) {
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, LOW);
    } else if (state == -1) {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, HIGH);
    } else {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, LOW);
    }
}

void buzz(int on_ms, int off_ms = 40) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(on_ms);
  digitalWrite(BUZZER_PIN, LOW);
  delay(off_ms);
}

void beepConnected() {
  buzz(80, 60);
  buzz(140, 0);
}

void beepDisconnected() {
  buzz(360, 580);
  buzz(80, 0);
}

int connectedState = 0;
void loop() {
    if (!Ps3.isConnected()) {
        connectedState = 0;
        digitalWrite(ONBOARD_LED, (millis() / 500) % 2);
        // Beep slowly if disconnected? Optional.
        beepDisconnected();
        return;
    }
    digitalWrite(ONBOARD_LED, HIGH);
    if (!connectedState) beepConnected();
    connectedState = 1;

    // ============================
    // 1. DRIVING LOGIC (Left Stick)
    // ============================
    int throttle = -Ps3.data.analog.stick.ly; 
    int steering = Ps3.data.analog.stick.lx;  

    if (abs(throttle) < 10) throttle = 0;
    if (abs(steering) < 10) steering = 0;

    // --- Reverse Beeper Logic ---
    if (throttle < -20) {
        // Blink Buzzer every 500ms
        if ((millis() / 300) % 2 == 0) {
            digitalWrite(BUZZER_PIN, HIGH);
        } else {
            digitalWrite(BUZZER_PIN, LOW);
        }
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    int leftSpeed = throttle + steering;
    int rightSpeed = throttle - steering;

    setDriveMotor(leftSpeed * 2, IN1, IN2, pwmChannelA);
    setDriveMotor(rightSpeed * 2, IN3, IN4, pwmChannelB);


    // ============================
    // 2. ARM LOGIC
    // ============================

    // --- Arm Motor 1: R2 (Fwd) / L2 (Rev) ---
    // Note: R2/L2 are analog buttons (0-255), we treat > 50 as pressed
    int arm1_state = 0;
    if (Ps3.data.analog.button.r2 > 50) arm1_state = 1;
    else if (Ps3.data.analog.button.l2 > 50) arm1_state = -1;
    setArmMotor(arm1_state, ARM1_IN1, ARM1_IN2);


    // --- Arm Motor 2: R1 (Fwd) / L1 (Rev) ---
    // R1/L1 are boolean buttons in some libs, but often analog in PS3. 
    // Ps3Controller lib usually exposes .button.r1 as bool? 
    // Let's check struct. actually Ps3.data.button.r1 is bool
    int arm2_state = 0;
    if (Ps3.data.button.r1) arm2_state = 1;
    else if (Ps3.data.button.l1) arm2_state = -1;
    setArmMotor(arm2_state, ARM2_IN1, ARM2_IN2);


    // --- Arm Motor 3: Triangle/Circle (Fwd) / Cross/Square (Rev) ---
    int arm3_state = 0;
    if (Ps3.data.button.triangle || Ps3.data.button.circle) arm3_state = 1;
    else if (Ps3.data.button.cross || Ps3.data.button.square) arm3_state = -1;
    setArmMotor(arm3_state, ARM3_IN1, ARM3_IN2);


    // --- Arm Motor 4: Right Stick (Up/Right = Fwd, Down/Left = Rev) ---
    int ry = -Ps3.data.analog.stick.ry; // Up is +, Down is -
    int rx = Ps3.data.analog.stick.rx;  // Right is +, Left is -
    
    int arm4_state = 0;
    // Simple logic: if Stick moved significantly in any direction
    if (ry > 50 || rx > 50) arm4_state = 1;      // Up or Right -> Forward
    else if (ry < -50 || rx < -50) arm4_state = -1; // Down or Left -> Backward
    setArmMotor(arm4_state, ARM4_IN1, ARM4_IN2);


    delay(10);
}