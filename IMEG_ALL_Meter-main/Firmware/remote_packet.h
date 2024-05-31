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
  int month = 12;
  int day = 29;
  int year = 2024;
  
  int rxBatt = 9;
    
};
enum rx_command {
	LUX_READ, READ_DONE, PWR_OFF
};

struct rx_packet {
	
	rx_command cmd = READ_DONE;
	
	uint8_t data[32] = {0};
}