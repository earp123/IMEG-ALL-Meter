#define CHANNEL 1

#include <esp_now.h>
#include <WiFi.h>

const uint8_t ReceiverMAC[] = {0xc4, 0xdd, 0x57, 0x67, 0x37, 0x84};

uint8_t startMsg[] = "ESP NOW GUI Has Started";

void ESPNOW_init(){

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t ReceiverInfo;
  memcpy(ReceiverInfo.peer_addr, ReceiverMAC, 6);
  ReceiverInfo.channel = CHANNEL;
  ReceiverInfo.ifidx = WIFI_IF_STA;
  ReceiverInfo.encrypt = false;

  esp_err_t result = esp_now_add_peer(&ReceiverInfo);
  if (result != ESP_OK)
    Serial.println("Failed to add peer");
  else
    Serial.println("Receiver added as ESP NOW Peer");

  result = esp_now_send(ReceiverMAC, (uint8_t *) &startMsg, sizeof(startMsg));
  Serial.print("Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
    // How did we get so far!!
    Serial.println("ESPNOW not Init.");
  } else if (result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("Invalid Argument");
  } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
    Serial.println("Internal Error");
  } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
    Serial.println("Peer not found.");
  } else {
    Serial.println("Not sure what happened");
  }
}

void send_txCommand(){
  esp_now_send(ReceiverMAC, (uint8_t *) &tx_cmd_packet, sizeof(tx_cmd_packet));
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  packetRx = true;
  lastPacketRx = millis();
  char macStr[18];

  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);


  memcpy(&GUIpacket, data, sizeof(GUIpacket));

  Serial.print("Last Packet Recv from: "); Serial.println(macStr);
  Serial.println();
  
}