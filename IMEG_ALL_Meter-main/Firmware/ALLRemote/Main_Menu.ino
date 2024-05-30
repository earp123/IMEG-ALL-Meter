static void updateMenu(int index)
{
  
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(3);

  if (index == 1) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("TIME ZONE SET");

  if (index == 2) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("FS MENU");

  if (index == 3) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("VEML7700 MENU");

  if (index == 4) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("RX MENU");

  if (index == 5) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("<BACK");

}

void mainMenu()
{
  int menu_idx = 1;

  updateMenu(menu_idx);

  while(1)
  {
    switch(butn){
      case ABUTN:
        butn = NONE;
        menu_idx++;
        if (menu_idx>5) menu_idx = 1;
        updateMenu(menu_idx);
        break;

      case BBUTN:
        butn = NONE;
        menu_idx--;
        if (menu_idx<1) menu_idx = 5;
        updateMenu(menu_idx);
        break;

      case CBUTN:
        butn = NONE;
        if (menu_idx == 5){
          M5.Lcd.clear();
          mainDisplay();
        } 
        break;
      case NONE:
        //fall through
      default: break;
    }
    delay(300);
  }
  
}
