#include <Arduino.h>
#include <Ps3Controller.h>
#include <WiFi.h>
#include <WebServer.h>

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
const int resolution = 8; 

// --- WIFI / WEB SERVER ---
const char* ssid = "Hydrozer_AP";
const char* password = "12312345";

WebServer server(80);

// --- MODES ---
enum ControlMode {
    MODE_SEARCHING,
    MODE_PS3,
    MODE_WIFI
};

ControlMode currentMode = MODE_SEARCHING;
unsigned long searchStartTime = 0;
const unsigned long SEARCH_TIMEOUT = 5000; // 5 Seconds wait for PS3

// --- HTML PAGE ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Hydrozer Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px; background-color: #222; color: white; }
    .btn { background-color: #444; border: none; color: white; padding: 15px 32px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; user-select: none; }
    .btn:active { background-color: #4CAF50; }
    .stop { background-color: #f44336; }
    .ctrl-group { margin-bottom: 20px; border: 1px solid #555; padding: 10px; border-radius: 10px; }
    h2 { margin-top: 0; }
  </style>
</head>
<body>
  <h1>Hydrozer RC</h1>
  
  <div class="ctrl-group">
    <h2>Drive</h2>
    <button class="btn" ontouchstart="move('F')" ontouchend="move('S')" onmousedown="move('F')" onmouseup="move('S')">FWD</button><br>
    <button class="btn" ontouchstart="move('L')" ontouchend="move('S')" onmousedown="move('L')" onmouseup="move('S')">LEFT</button>
    <button class="btn stop" ontouchstart="move('S')" onmousedown="move('S')">STOP</button>
    <button class="btn" ontouchstart="move('R')" ontouchend="move('S')" onmousedown="move('R')" onmouseup="move('S')">RIGHT</button><br>
    <button class="btn" ontouchstart="move('B')" ontouchend="move('S')" onmousedown="move('B')" onmouseup="move('S')">REV</button>
  </div>

  <div class="ctrl-group">
    <h2>Arms</h2>
    <p>Arm 1: <button class="btn" onclick="act('1')">UP</button> <button class="btn" onclick="act('q')">DN</button> <button class="btn stop" onclick="act('x')">X</button></p>
    <p>Arm 2: <button class="btn" onclick="act('2')">UP</button> <button class="btn" onclick="act('w')">DN</button> <button class="btn stop" onclick="act('y')">X</button></p>
    <p>Arm 3: <button class="btn" onclick="act('3')">UP</button> <button class="btn" onclick="act('e')">DN</button> <button class="btn stop" onclick="act('z')">X</button></p>
    <p>Arm 4: <button class="btn" onclick="act('4')">UP</button> <button class="btn" onclick="act('r')">DN</button> <button class="btn stop" onclick="act('v')">X</button></p>
  </div>

<script>
function move(dir) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/cmd?go=" + dir, true);
  xhr.send();
}
function act(action) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/cmd?act=" + action, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// --- WEB HANDLERS ---
void handleRoot() {
    server.send(200, "text/html", index_html);
}

void handleCmd() {
    if (server.hasArg("go")) {
        String go = server.arg("go");
        if (go == "F") { setDriveMotor(255, IN1, IN2, pwmChannelA); setDriveMotor(255, IN3, IN4, pwmChannelB); }
        else if (go == "B") { setDriveMotor(-255, IN1, IN2, pwmChannelA); setDriveMotor(-255, IN3, IN4, pwmChannelB); }
        else if (go == "L") { setDriveMotor(-255, IN1, IN2, pwmChannelA); setDriveMotor(255, IN3, IN4, pwmChannelB); }
        else if (go == "R") { setDriveMotor(255, IN1, IN2, pwmChannelA); setDriveMotor(-255, IN3, IN4, pwmChannelB); }
        else if (go == "S") { setDriveMotor(0, IN1, IN2, pwmChannelA); setDriveMotor(0, IN3, IN4, pwmChannelB); }
    }
    
    if (server.hasArg("act")) {
        char act = server.arg("act").charAt(0);
        switch(act) {
            case '1': setArmMotor(1, ARM1_IN1, ARM1_IN2); break; case 'q': setArmMotor(-1, ARM1_IN1, ARM1_IN2); break; case 'x': setArmMotor(0, ARM1_IN1, ARM1_IN2); break;
            case '2': setArmMotor(1, ARM2_IN1, ARM2_IN2); break; case 'w': setArmMotor(-1, ARM2_IN1, ARM2_IN2); break; case 'y': setArmMotor(0, ARM2_IN1, ARM2_IN2); break;
            case '3': setArmMotor(1, ARM3_IN1, ARM3_IN2); break; case 'e': setArmMotor(-1, ARM3_IN1, ARM3_IN2); break; case 'z': setArmMotor(0, ARM3_IN1, ARM3_IN2); break;
            case '4': setArmMotor(1, ARM4_IN1, ARM4_IN2); break; case 'r': setArmMotor(-1, ARM4_IN1, ARM4_IN2); break; case 'v': setArmMotor(0, ARM4_IN1, ARM4_IN2); break;
        }
    }
    server.send(200, "text/plain", "OK");
}

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
    searchStartTime = millis();
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
    if (currentMode == MODE_SEARCHING) {
        digitalWrite(ONBOARD_LED, (millis() / 100) % 2); // Fast Blink
        
        if (Ps3.isConnected()) {
            Serial.println("PS3 Connected!");
            currentMode = MODE_PS3;
            digitalWrite(BUZZER_PIN, HIGH); delay(500); digitalWrite(BUZZER_PIN, LOW);
        } else if (millis() - searchStartTime > SEARCH_TIMEOUT) {
            Serial.println("Timeout. Starting WiFi AP...");
            Ps3.end(); // Stop Bluetooth Stack
            
            // Start WiFi AP
            WiFi.softAP(ssid, password);
            IPAddress myIP = WiFi.softAPIP();
            Serial.print("AP IP address: "); Serial.println(myIP);
            
            server.on("/", handleRoot);
            server.on("/cmd", handleCmd);
            server.begin();
            
            currentMode = MODE_WIFI;
            // 3 Short Beeps
             for(int i=0; i<3; i++) { digitalWrite(BUZZER_PIN, HIGH); delay(50); digitalWrite(BUZZER_PIN, LOW); delay(50); }
        }
    }
    else if (currentMode == MODE_PS3) {
        digitalWrite(ONBOARD_LED, HIGH);
        // ... PS3 Logic from before ...
        if (!Ps3.isConnected()) {
            // Handle disconnect?
        }
        
        // Drive Logic
        int throttle = -Ps3.data.analog.stick.ly; 
        int steering = Ps3.data.analog.stick.lx;  
        if (abs(throttle) < 10) throttle = 0; if (abs(steering) < 10) steering = 0;

        // Reverse Beeper
        if (throttle < -20 && (millis()/300)%2==0) digitalWrite(BUZZER_PIN, HIGH);
        else digitalWrite(BUZZER_PIN, LOW);

        setDriveMotor((throttle+steering)*2, IN1, IN2, pwmChannelA);
        setDriveMotor((throttle-steering)*2, IN3, IN4, pwmChannelB);

        // Arm Logic
        int a1=0; if(Ps3.data.analog.button.r2>50)a1=1; else if(Ps3.data.analog.button.l2>50)a1=-1; setArmMotor(a1, ARM1_IN1, ARM1_IN2);
        int a2=0; if(Ps3.data.button.r1)a2=1; else if(Ps3.data.button.l1)a2=-1; setArmMotor(a2, ARM2_IN1, ARM2_IN2);
        int a3=0; if(Ps3.data.button.triangle||Ps3.data.button.circle)a3=1; else if(Ps3.data.button.cross||Ps3.data.button.square)a3=-1; setArmMotor(a3, ARM3_IN1, ARM3_IN2);
        
        int a4=0; int ry=-Ps3.data.analog.stick.ry; int rx=Ps3.data.analog.stick.rx;
        if(ry>50||rx>50)a4=1; else if(ry<-50||rx<-50)a4=-1; setArmMotor(a4, ARM4_IN1, ARM4_IN2);
        
        delay(10);
    }
    else if (currentMode == MODE_WIFI) {
        digitalWrite(ONBOARD_LED, (millis() / 1000) % 2); // Slow Blink
        server.handleClient();
        delay(2);
    }
}
