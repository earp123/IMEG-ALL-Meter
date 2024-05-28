struct remote_packet {
  uint16_t lux = 45;
  uint8_t satsInView = 23;
  float horizAcc = 0.02;
  
  //use 8 decminals
  double latit  = 45.12345678;
  double longit = 23.87654321;
  
  //from Rx RTC
  int hour = 12;
  int minute = 34;
  
  int rxBatt = 9;
  
  //need to clear after we capture it
  bool luxCapture = false;
 
  
};