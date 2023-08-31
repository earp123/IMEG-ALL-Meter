struct {

  int batt;
  int SatsInView;
  float HorizAcc;
  float lux;
  double latitude;
  double longitude;

  bool timeAvail;
  bool sysON;
  bool bt_connected;

  //TODO Day, Month, Year

  int hour_s;
  int min_s;
  int sec_s;

} GUIpacket;

struct {

  bool pwr_down;
  bool pause_tx;

} tx_cmd_packet;