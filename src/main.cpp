#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <string.h>

HardwareSerial serial(0);

void setup() {
  serial.begin(9600);
}

void loop() {
  serial.println("Hello, World!");
  delay(2000);
}
