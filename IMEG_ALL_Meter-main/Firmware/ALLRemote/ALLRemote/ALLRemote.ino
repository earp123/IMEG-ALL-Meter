#include <esp_now.h>
#include <WiFi.h>

#include "M5Unified.h"
#include "remote_packet.h"

#define CHANNEL 1

struct remote_packet incoming_p;

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    //M5.Lcd.println("ESPNow Init Success");
  }
  else {
    M5.Lcd.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "ALLRemote";
  bool result = WiFi.softAP(SSID, "imegallremote", CHANNEL, 0);
  if (!result) {
    M5.Lcd.println("AP Config failed.");
  } else {
    //M5.Lcd.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

void setup() {
  M5.Lcd.begin();
  
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  M5.Lcd.print("AP MAC: "); M5.Lcd.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  
  M5.Lcd.setTextSize(4);  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //M5.Lcd.print("Last Packet Recv from: "); M5.Lcd.println(macStr);
  
  memcpy(&incoming_p, data, data_len);
  
}

void loop() {

  M5.Lcd.setCursor(0, 10);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("          ");
  M5.Lcd.setCursor(0, 10);

  if(incoming_p.lux < 65535) M5.Lcd.println(incoming_p.lux);
  else                       M5.Lcd.println("Unstable");

  delay(2000);
}
