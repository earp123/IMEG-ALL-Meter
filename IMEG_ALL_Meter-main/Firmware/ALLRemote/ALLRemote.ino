#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>

#include <M5Unified.h>

#include "remote_packet.h"

//TODO get SD card working

#define CHANNEL 1

struct remote_packet incoming_p;
File myFile;

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

  M5.begin();
  Serial.begin(115200);
  if (!SD.begin(4, SPI, 4000000)) {  // Initialize the SD card. 初始化SD卡
    M5.Lcd.println(
        "Card failed, or not present");
    while (1)
        ;
  }
  M5.Lcd.println("TF card initialized.");
  myFile = SD.open("/hello.txt", FILE_WRITE, true);  // Create a new file "/hello.txt".
  myFile.close();

  
    
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print("AP MAC: "); M5.Lcd.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();

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

  M5.update();
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(4);  
  M5.Lcd.println("          ");
  M5.Lcd.setCursor(0, 20);

  if(incoming_p.lux < 65535){
    M5.Lcd.println(incoming_p.lux);
  } 
  else M5.Lcd.println("Unstable");

  M5.Lcd.setCursor(0,60);
  M5.Lcd.setTextSize(1);
  if (M5.BtnA.isPressed()) {
    
    myFile = SD.open("/hello.txt", FILE_WRITE);
    if (myFile){
      myFile.println("Testing multiple lines with \n carriage return.");
      myFile.println(incoming_p.lux, DEC);
      incoming_p.lux++;
      myFile.println(incoming_p.lux, DEC);
      myFile.close();
      
      M5.Lcd.println("Value written to SD");
    }
  }else{
    M5.Lcd.println("                    ");
  }

  delay(1000);
}
