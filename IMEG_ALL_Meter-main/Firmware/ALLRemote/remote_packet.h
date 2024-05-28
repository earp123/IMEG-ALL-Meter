struct remote_packet {
  uint16_t lux = 45;
  uint8_t satsInView;
  float horizAcc;
  
  //use 8 decminals
  double latit;
  double longit;
  
  //from Rx RTC
  int hour;
  int minute;
  
  int rxBatt;
  
  //need to clear after we capture it
  bool luxCapture = false;
  
  
  bool connected = true;
  
};