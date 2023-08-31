/* 
*  Displays data from a Master ESP32 Device using ESPNOW
*
*  
*  TFT Display GUI meant for use with this product 
*  https://www.amazon.com/gp/product/B073R7BH1B/ref=ppx_yo_dt_b_search_asin_title?ie=UTF8&psc=1
*
*  HiLetgo ILI9341 2.8" SPI TFT LCD Display Touch Panel 240X320 with PCB 5V/3.3V STM32
*
*  By Sam Rall ~SWR~
*/
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>    // Core graphics library

#include <XPT2046_Touchscreen.h>

#include <esp_now.h>
#include <WiFi.h>
#include "packet_defs.hpp"

#include "SdFat.h"
#include "sdios.h"


// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

/* Currently using Sparkfun ESP32-S2 Thing Plus
*  These pins are all on the same side of the board
*  It may require the RST pins on both the TFT and ESP32 board to be connected
*/

//~SWR~ DO NOT CHANGE THESE
#define IRQ_PIN A5
#define XPT_CS  3
#define TFT_CS 34
#define TFT_DC 33
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
XPT2046_Touchscreen ts(XPT_CS, IRQ_PIN);

// Size of the selection boxes and the paintbrush size
#define BOXSIZE 30

const uint8_t SD_CS_PIN = A4;

#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(20))

SdFat32 sd;
File32 dataFile;

#define WAIT_FOR_PACKET 16000

//easy index for the colors - must be in specific order
uint16_t color_idx[] = {ILI9341_GREEN, ILI9341_CYAN, ILI9341_BLUE, 
                        ILI9341_YELLOW, ILI9341_RED, ILI9341_MAGENTA};


int lastPacketRx = 0;
bool packetRx = false;
bool noData = false;

int tz_hour = 0; //need this as global for data logging. 



#define MAIN_MENU     1
#define OFF_MENU      2 
#define LOG_MENU      3
#define SEND_CMD_MENU 4

uint8_t MENU_STATE = MAIN_MENU;


void setup(void) {
 
  Serial.begin(115200);

  //For FAST OFF functionality
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  ESPNOW_init();
  
  tft.begin();
  tft.setRotation(2);
  tft.setCursor(0,0);

  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  ts.setRotation(0);
  Serial.println("Touchscreen started");

  tft.fillScreen(ILI9341_BLACK);
  
  drawMainMenu();

}

void loop()
{

  if (millis() - lastPacketRx <= WAIT_FOR_PACKET)
  {
    displayGUIdata();
    packetRx = false;
    noData = false;
  }
  else
  {
    if(!noData)
    {
      tft.fillRect(0, 0, tft.width(), tft.height()*6/10, ILI9341_BLACK); 
      tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      tft.setTextSize(2); 
      tft.setCursor(0,0);
      tft.println("NO DATA");
    }
    noData = true;
  }
  
  // You can also wait for a touch
  if (! ts.touched()) {
    return;
  }
  
  // Retrieve a point  
  TS_Point p = ts.getPoint();
 
  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  //Select the option
  if (p.y < (tft.height()-BOXSIZE*2)){
    //Do nothing
  }
  else{

    switch(MENU_STATE){
      case MAIN_MENU:
      {
        if (touchInZone1_1(p.x, p.y)){
          //Option 1,1
          drawOffMenu();
          MENU_STATE = OFF_MENU;
          break;
        }
        else if (touchInZone2_1(p.x, p.y)){
          //Option 2, 1
          if (!noData)
          {
            drawLogMenu();
            MENU_STATE = LOG_MENU;
          }
          else
          {
            tft.setTextSize(2);
            tft.setCursor(0, tft.height()-(3*BOXSIZE));
            tft.print("NO DATA TO LOG");
            delay(1500);
            drawMainMenu();
          }

          break;
        }
          
        else if (touchInZone1_2(p.x, p.y)){
          //Option 1, 2
          //Update MENU_STATE
          //TX CMD
          break;
        }
          
        else if (touchInZone2_2(p.x, p.y)){
          //Option 2,2
          //Update MENU_STATE
          break;
        }
        else break;
      }
      case OFF_MENU:
      {
        if(touchInZone1_1(p.x, p.y))
        {
          //Power OFF
          digitalWrite(12, LOW);
        }
        else if(touchInZone2_1(p.x, p.y))
        {
          drawMainMenu();
          MENU_STATE = MAIN_MENU;
          
        } 
        break;
      }
      case LOG_MENU:
      {
        //TODO catch all the errors and scenarios
        if (touchInZone1_1(p.x, p.y)){
          //Option 1, 1
          //Update MENU_STATE
          int ret = logPacket();
          if (ret == 0)
          {
            tft.setTextSize(2);
            tft.setCursor(0, tft.height()-(3*BOXSIZE));
            tft.print("GPS PT LOGGED!");
            drawMainMenu();
            MENU_STATE = MAIN_MENU;
          }
          else
          {
            tft.setTextSize(2);
            tft.setCursor(0, tft.height()-(3*BOXSIZE));
            tft.print("ERROR "); tft.println(ret);
            drawMainMenu();
            MENU_STATE = MAIN_MENU;
          }
        
          break;
        }
          
        else if (touchInZone2_1(p.x, p.y)){
          //Option 2,1
          //Update MENU_STATE
          drawMainMenu();
          MENU_STATE = MAIN_MENU;
          break;
        }
      }

      case SEND_CMD_MENU:
      default: break;

    }


  }
}

void drawMainMenu()
{
  //Clear Menu Space
  tft.fillRect(0, tft.height()-(BOXSIZE*2), tft.width(), BOXSIZE*2, ILI9341_BLACK);

  //Draw Option 1,1
  tft.fillRect(0, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[0]);

  //Label Option 1,1
  tft.setTextSize(2);
  tft.setCursor((BOXSIZE+5), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[0], ILI9341_BLACK);  
  tft.print("Pwr OFF");

  //Draw Option 2,1
  tft.fillRect(tft.width()/2, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[1]);

  //Label Option 2,1
  tft.setTextSize(2);
  tft.setCursor(tft.width()/2+(BOXSIZE+5), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[1], ILI9341_BLACK);  
  tft.print("Log Pt");

  //Draw Option 1,2
  tft.fillRect(0, (tft.height()-BOXSIZE*2), BOXSIZE, BOXSIZE, color_idx[2]);

  //Label Option 1,2
  tft.setTextSize(2);
  tft.setCursor((BOXSIZE+5), tft.height()-2*BOXSIZE+4);
  tft.setTextColor(color_idx[2], ILI9341_BLACK);  
  tft.print("Tx CMD");

  //TODO File Select menu, Option 2,2


}

void drawOffMenu(){

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setCursor(0, tft.height()-(BOXSIZE+3*BOXSIZE/4));
  tft.println("Power Down?");

  tft.fillRect(0, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[3]);
  tft.setCursor((BOXSIZE+10), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[3], ILI9341_BLACK);  
  tft.print("Yes");

  tft.fillRect(tft.width()/2, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[0]);
  tft.setCursor(tft.width()/2+(BOXSIZE+10), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[0], ILI9341_BLACK);  
  tft.print("No");

}

void drawLogMenu(){

  //Clear Menu Space
  tft.fillRect(0, tft.height()-(BOXSIZE*2), tft.width(), BOXSIZE*2, ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setCursor(0, tft.height()-(BOXSIZE+3*BOXSIZE/4));
  tft.println("Log GPS Point?");

  tft.fillRect(0, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[3]);
  tft.setCursor((BOXSIZE+10), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[3], ILI9341_BLACK);  
  tft.print("Yes");

  tft.fillRect(tft.width()/2, (tft.height()-BOXSIZE), BOXSIZE, BOXSIZE, color_idx[0]);
  tft.setCursor(tft.width()/2+(BOXSIZE+10), tft.height()-(3*BOXSIZE/4));
  tft.setTextColor(color_idx[0], ILI9341_BLACK);  
  tft.print("No");

}

void displayGUIdata()
{
  //Print to TFT Display
  //GPS coordinates

  tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft.setTextSize(2); 
  tft.setCursor(0,0); 
  tft.println(GUIpacket.latitude, 8);
  tft.println(GUIpacket.longitude, 8);

  //Satellites in View, Horizontal Accuracy
  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print("SIV:"); tft.print(GUIpacket.SatsInView); tft.print(" Acc:");
  if (GUIpacket.HorizAcc > 1000)
  {
    tft.println("--      ");
  }
  else {
     tft.print(GUIpacket.HorizAcc, 3); tft.println("m");
  }

  //Lux data
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setTextSize(4);
  if (GUIpacket.lux<0){
    tft.print(" ... ");
  }
  else{
    float lux_raw = GUIpacket.lux;
    //temp correction for the lensing. TODO create a setting to turn this on and off
    float lux_lensed = lux_raw*3.5;
    tft.print(lux_lensed, 1);
  } 
  tft.println(" Lux");
  

  //Time from Rx RTC
  tft.setTextSize(3); tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  if (GUIpacket.timeAvail)
  {
    tz_hour = GUIpacket.hour_s+6;
    if (tz_hour < 10) tft.print(" ");
    tft.print(tz_hour);
    tft.print(":");
    if (GUIpacket.min_s < 10) tft.print("0");
    tft.print(GUIpacket.min_s);
    tft.println(" MDT");
  }
  else
    tft.println(" Time NA   ");

  //Battery level of the Rx
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print("Rx Batt: "); tft.print(GUIpacket.batt);tft.println("%");

  tft.setTextColor(ILI9341_BLUE, ILI9341_BLACK);
  if (GUIpacket.bt_connected)
    tft.println("BT Connected");
  else
    tft.println("            ");

}

int logPacket(){

  int ret = 0;

  //Switch off screen and touch controller pins momentarily to begin SD card
  pinMode(TFT_DC, OUTPUT);
  digitalWrite(TFT_DC, HIGH);


  pinMode(XPT_CS, OUTPUT);
  digitalWrite(XPT_CS, HIGH);

  if (!sd.begin(SD_CONFIG)) {
    Serial.println(
            "\nSD initialization failed.\n"
            "Do not reformat the card!\n"
            "Is the card correctly inserted?\n"
            "Is there a wiring/soldering problem?\n");
    if (isSpi(SD_CONFIG)) {
      Serial.println(
            "Is SD_CS_PIN set to the correct value?\n"
            "Does another SPI device need to be disabled?\n"
            );
      ret = 2;
    }
    else ret = 1;

    return ret;
 
  }

  else // SD Card present, init sucessfull
  {
    
    //TODO File names as YYYYMMDD_HHMMSS.txt
    if (!dataFile.open("YYYYMMDD_HHMMSS.txt", O_RDWR | O_CREAT | O_APPEND)) 
    {
      Serial.println("Couldn't create text file");
      ret = 3;
    }
    else //File created/accessed 
    {
      //Log GPS + Lux Point
      dataFile.println(GUIpacket.latitude, 8);
      dataFile.println(GUIpacket.longitude, 8);
      dataFile.println(GUIpacket.lux);
      //Time Stamp
      dataFile.print(tz_hour); dataFile.print(GUIpacket.min_s);dataFile.println(GUIpacket.sec_s);
      dataFile.println();

      dataFile.close();
      Serial.println("SD OK");
    }
  }

  return ret;

}

bool touchInZone1_1(uint16_t p_x, uint16_t p_y){
  if ((p_x < tft.width()/2) && (p_y > tft.height()-BOXSIZE))
    return true;
  else 
    return false;
}

bool touchInZone1_2(uint16_t p_x, uint16_t p_y){
  if ((p_x < tft.width()/2) && (p_y < tft.height()-BOXSIZE))
    return true;
  else 
    return false;
}

bool touchInZone2_1(uint16_t p_x, uint16_t p_y){
  if ((p_x > tft.width()/2) && (p_y > tft.height()-BOXSIZE))
    return true;
  else 
    return false;
}

bool touchInZone2_2(uint16_t p_x, uint16_t p_y){
  if ((p_x > tft.width()/2) && (p_y < tft.height()-BOXSIZE))
    return true;
  else 
    return false;
}

