void init_label(int x_pos, int y_pos, int fg_color, int bg_color, float text_size, String show_text)
{
  M5.Lcd.setCursor(x_pos, y_pos);
  M5.Lcd.setTextSize(text_size);
  M5.Lcd.setTextColor(fg_color, bg_color);
  M5.Lcd.print(show_text);

}

void updateMainDisplay()
{
  //receiver Battery
  if (connected) init_label(10, 0, BLACK, GREEN, 3, "  ");
  init_label(12, 2, (connected ? BLACK: WHITE), (connected ? GREEN: BLACK), 2.5, "Rx");
  String rx_batt = "";
  rx_batt.concat(incoming_p.rxBatt);
  rx_batt.concat("% ");

  init_label(60, 2, GREEN, BLACK, 2.5, rx_batt);

  //Time
  String display_time = "";
  if (incoming_p.hour < 10) display_time.concat(" ");
  display_time += incoming_p.hour;
  display_time += ":";
  if (incoming_p.minute < 10) display_time.concat("0");
  display_time += incoming_p.minute; 
  init_label(225, 0, RED, BLACK, 3, display_time);

  //SIV
  String display_siv = "SIV:";
  display_siv += incoming_p.satsInView;
  display_siv += " ";//covers up the last digit if we go <10
  init_label(10, 50, WHITE, BLACK, 3, display_siv);

  //Horizontal Accuracy
  String display_hza = "Acc:";
  display_hza += incoming_p.horizAcc;
  display_hza += "m";
  init_label(160, 50, WHITE, BLACK, 2, display_hza);

  //Lat, Long
  String display_lat = "LAT:";
  init_label(10, 80, WHITE, BLACK, 2, display_lat);
  M5.Lcd.print(incoming_p.latit, 8);//just use the print settings from the above init Fn

  String display_long = "LONG:";
  init_label(10, 100, WHITE, BLACK, 2, display_long);
  M5.Lcd.print(incoming_p.longit, 8);//just use the print settings from the above init Fn

  //Last Lux
  String display_lux = "Last: ";
  display_lux += incoming_p.lux;
  display_lux += " Lux";
  init_label(10, 120, YELLOW, BLACK, 3, display_lux);

  //Button Graphics
  init_label(30, 216, BLACK, BLUE, 3, "    ");
  init_label(125, 216, BLACK, BLUE, 3, "    ");
  init_label(220, 216, BLACK, BLUE, 3, "    ");

  //Button Labels
  init_label(35, 220, BLACK, BLUE, 2.5, "READ");
  init_label(140, 220, BLACK, BLUE, 2.5, "LOG");
  init_label(225, 220, BLACK, BLUE, 2.5, "MENU");

   
}