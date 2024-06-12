#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define DELETE_OP 1
#define RESUME_OP 2

void resumeSurvey(int sel)
{
  File root = SD.open("/surveys");
  File file = root.openNextFile();

  for (int i = 1; i < sel; i++)
  {
    file = root.openNextFile();
  }

  currentLogFilePath = file.path();
  currentLogFileName = file.name();
}

int updateSurveySelect(fs::FS &fs, int index){
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextSize(3);

    File root = fs.open("/");
    
    File file = root.openNextFile();
    int count = 0;
    while(file){
      if(file.isDirectory())
      {
          // do nothing
      } 
      else 
      {
        if (index == (count+1)) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
        else                    M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.println(file.name());
      }
      file = root.openNextFile();
      count++;
    }

    return count;
}

void surveySelect(int fileOp)
{
  int menu_idx = 1;
  int numFiles = updateSurveySelect(SD, menu_idx);
  if (numFiles == 0) 
    return;
  int display_time = millis();
  while((millis() - display_time) < (MENU_TIMEOUT_S*1000))
  {
    switch(butn){
        case ABUTN:
          butn = NONE;
          menu_idx++;
          if (menu_idx >= numFiles) menu_idx = 1;
          updateSurveySelect(SD, menu_idx);
          display_time = millis();
          break;

        case BBUTN:
          butn = NONE;
          menu_idx--;
          if (menu_idx < 1) menu_idx = (numFiles-1);
          updateSurveySelect(SD, menu_idx);
          display_time = millis();
          break;

        case CBUTN:
          butn = NONE;
          if (fileOp == RESUME_OP)
          {
            resumeSurvey(menu_idx);
            M5.Lcd.clear();
            init_label(10, 10, WHITE, BLACK, 2, "Current Survey is now:\n");
            M5.Lcd.println(currentLogFileName);
            delay(3000);
            M5.Lcd.clear();
            mainDisplay();
          } 
          else if (fileOp == DELETE_OP)
          {
            deleteSurvey(menu_idx);
            delay(3000);
            M5.Lcd.clear();
            FSmenu();
          }
          break;
        case NONE:
          //fall through
        default: break;
      }
  }
}

void newSurvey(fs::FS &fs)
{
  String fileName = "/surveys/";
  
  fileName.concat(incoming_p.month);
  fileName.concat(incoming_p.day);
  fileName.concat(incoming_p.year);
  fileName.concat("_");
  fileName.concat(incoming_p.hour);
  fileName.concat(incoming_p.minute);
  fileName.concat(".csv");
  File newSurveyFile = fs.open(fileName, FILE_WRITE, true);
  if (newSurveyFile)
  {
    currentLogFilePath = newSurveyFile.path();
    currentLogFileName = newSurveyFile.name();

    Serial.print("Current Log Path: ");
    Serial.println(newSurveyFile.path());
  }
  else 
    Serial.println("Error creating file");
  newSurveyFile.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

bool logPoint(fs::FS &fs, String path, uint16_t luxVal, double latittude, double longitude){
    
    bool retVal;
    
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return false;
    }
    if(file.print(luxVal) && file.print(", ") && 
        file.print(latittude, 8) && file.print(", ") &&
        file.print(longitude, 8) && file.println()){
        Serial.println("Data Point Logged");
        retVal = true;
    } else {
        Serial.println("Append failed");
        retVal = false;
    }
    file.close();

    return retVal;
}

void deleteSurvey(int sel){

  File file;
  
  for (int i = 1; i < sel; i++)
  {
    file = sdroot.openNextFile();
  }
  
  SD.remove(file.path());
  M5.Lcd.clear();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println(file.name());
  M5.Lcd.println("Deleted");
}

void updateFSmenu(int index)
{
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(3);

  if (index == 1) M5.Lcd.setTextColor(BLACK, WHITE);//Highlight the current selection
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("NEW SURVEY");

  if (index == 2) M5.Lcd.setTextColor(BLACK, WHITE);
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("RESUME SURVEY");

  if (index == 3) M5.Lcd.setTextColor(BLACK, WHITE);
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("DELETE SURVEY");

  if (index == 4) M5.Lcd.setTextColor(BLACK, WHITE);
  else            M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.println("<BACK");
}

void FSmenu()
{
   int menu_idx = 1;

  updateFSmenu(menu_idx);

  int display_time = millis();
  while((millis() - display_time) < (MENU_TIMEOUT_S*1000))
  {
    switch(butn){
      case ABUTN:
        butn = NONE;
        menu_idx++;
        if (menu_idx>4) menu_idx = 1;
        updateFSmenu(menu_idx);
        display_time = millis();
        break;

      case BBUTN:
        butn = NONE;
        menu_idx--;
        if (menu_idx<1) menu_idx = 4;
        updateFSmenu(menu_idx);
        display_time = millis();
        break;

      case CBUTN:
        butn = NONE;
        if (menu_idx == 4){//BACK
          M5.Lcd.clear();
          mainMenu();
        }
        else if (menu_idx == 3){//DELETE
          M5.Lcd.clear();
          surveySelect(DELETE_OP);          
        }
        else if (menu_idx == 2){//RESUME
          M5.Lcd.clear();
          surveySelect(RESUME_OP);
          
        }
        else if (menu_idx == 1){//NEW
          M5.Lcd.clear();
          //TODO
          //SD writes do NOT like this thread context
          //Need to either take it back to the main loop or pass a semaphore
        }

        break;
      case NONE:
        //fall through
      default: break;
    }
    delay(300);
  }
  M5.Lcd.clear();
}