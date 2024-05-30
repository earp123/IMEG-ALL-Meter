#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>

#include <M5Unified.h>
#include "remote_packet.h"

#define CHANNEL 1

bool connected = false;
int lastPacket_s = 6;
struct remote_packet incoming_p;
File myFile;

enum buttonPress {
  ABUTN, BBUTN, CBUTN, NONE
};

enum menus {
  MN_DISPLAY, MN_MENU, FS_MENU
};

buttonPress butn = NONE;

void IRAM_ATTR Apress() {
    butn = ABUTN;
}

void IRAM_ATTR Bpress() {
    butn = BBUTN;
}

void IRAM_ATTR Cpress() {
    butn = CBUTN;
}

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

// SWR don't think I need this
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

  M5.begin();
  Serial.begin(115200);
  if (!SD.begin(4, SPI, 4000000)) {  
    M5.Lcd.println(
        "Card failed, or not present");
    while (1)
        ;
  }

  attachInterrupt(39, Apress, FALLING);
  attachInterrupt(38, Bpress, FALLING);
  attachInterrupt(37, Cpress, FALLING);
  
    
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  //M5.Lcd.print("AP MAC: "); M5.Lcd.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

  mainDisplay();

}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //M5.Lcd.print("Last Packet Recv from: "); M5.Lcd.println(macStr);
  memcpy(&incoming_p, data, data_len);
  
  lastPacket_s = 0;
  
}

void loop() {

  
}
