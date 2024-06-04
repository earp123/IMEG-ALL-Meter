#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>
#include "FS.h"

#include <M5Unified.h>
#include "remote_packet.h"

#define CHANNEL 1

volatile int GMToffset = 0;
volatile int surveyIdx = 1;
bool connected = false;
int lastPacket_s = 101;

File sdroot;
File currentLogFile;

const uint8_t rxMAC[6] = {0xEC, 0x64, 0xC9, 0x06, 0x12, 0x44};
esp_now_peer_info_t ALLReceiver;
struct remote_packet incoming_p;
struct rx_packet command_p;
esp_err_t result;
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
  M5.Power.begin();
  M5.Rtc.begin();
  Serial.begin(115200);

  attachInterrupt(39, Apress, FALLING);
  attachInterrupt(38, Bpress, FALLING);
  attachInterrupt(37, Cpress, FALLING);
  

  //Set device in AP mode to begin with
  WiFi.mode(WIFI_STA);
  // configure device AP mode
  configDeviceAP();
  // Init ESPNow with a fallback logic
  InitESPNow();

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(ALLReceiver.peer_addr, rxMAC, 6);
  ALLReceiver.channel = CHANNEL;
  ALLReceiver.ifidx = WIFI_IF_STA;
  result = esp_now_add_peer(&ALLReceiver);
  if (result != ESP_OK)
  {
    Serial.println("Failed to add peer");
  }

  if (!SD.begin(4, SPI, 4000000)) {  
    M5.Lcd.println(
        "Card failed, or not present");
    while (1)
        ;
  }

  sdroot = SD.open("/");
  currentLogFile = sdroot.openNextFile();

  M5.Rtc.begin();

  mainDisplay();

}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  
  memcpy(&incoming_p, data, data_len);
  
  lastPacket_s = 0;
  
}

void loop() {

  
  M5.Lcd.setBrightness(0);

  while (butn == NONE)
  {
    M5.Power.lightSleep(300000);
    delay(10);//hoping this feeds the RTC watch dog
  }
  
  M5.Lcd.setBrightness(80);

  //Set device in AP mode to begin with
  WiFi.mode(WIFI_STA);

  // configure device AP mode
  configDeviceAP();

  // Init ESPNow with a fallback logic
  InitESPNow();

  mainDisplay();
  
}
