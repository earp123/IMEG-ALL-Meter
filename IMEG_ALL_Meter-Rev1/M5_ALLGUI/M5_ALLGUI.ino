#include <M5Stack.h>

#include <esp_now.h>
#include <WiFi.h>

#include "packet_defs.hpp"

int lastPacketRx = 0;

bool idle = false;


void setup() {
    M5.begin();        // Init M5Core.  初始化 M5Core
    M5.Power.begin();  // Init Power module.  初始化电源模块
    /* Power chip connected to gpio21, gpio22, I2C device
      Set battery charging voltage and current
      If used battery, please call this function in your project */

    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(8);

    ESPNOW_init();

}


void loop() {

  if (millis() - lastPacketRx <= 1000)
  {
    M5.Lcd.setTextColor(BLUE, BLACK);
    if (GUIpacket.lux > 0) M5.Lcd.drawFloat(GUIpacket.lux, 2, 50, 80);
    idle = false;
  }
  else if (millis() - lastPacketRx <= 15000)
  {
    if (!idle)
    {
      M5.Lcd.setTextColor(WHITE, BLACK);
      if (GUIpacket.lux > 0) M5.Lcd.drawFloat(GUIpacket.lux, 2, 50, 80);
      idle = true;
    }
    else delay(100);
  }
  else
  {
    M5.Lcd.setTextColor(RED, BLACK);
    M5.Lcd.drawString("NO RX  ", 50, 80);
  }


}