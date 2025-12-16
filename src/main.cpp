#include <Arduino.h>
#include <Ps3Controller.h>

void setup()
{
  Serial.begin(115200);
  Ps3.begin("9C:B6:D0:DD:9D:10");
  Serial.println("PS3 Controller Initialized. Waiting for connection...");
}

void loop()
{
  if (Ps3.isConnected())
  {
    Serial.println("Controller Connected!");

    // Read Left Stick
    int ly = Ps3.data.analog.stick.ly;
    int lx = Ps3.data.analog.stick.lx;

    // Read Right Stick
    int ry = Ps3.data.analog.stick.ry;
    int rx = Ps3.data.analog.stick.rx;

    Serial.printf("LX: %4d, LY: %4d || RX: %4d, RY: %4d\n", lx, ly, rx, ry);

    if (Ps3.data.button.cross)
      Serial.println("Pressing Cross");
    if (Ps3.data.button.square)
      Serial.println("Pressing Square");
    if (Ps3.data.button.triangle)
      Serial.println("Pressing Triangle");
    if (Ps3.data.button.circle)
      Serial.println("Pressing Circle");
  }
  else
  {
    Serial.println("Waiting for controller...");
  }

  delay(100);
}