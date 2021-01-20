// include the SD library:
#include <SD.h>
#include <SPI.h>

// For display
#include <rgb_lcd.h>
#include <Wire.h>

/*
  Flash Cards application with Teensy 3.6 and 
  Grove - 2x16 LCD RGB Backlight.

  Copyright 2021 Tom Dison
  Licensed under GPL 3
  https://gnu.org/licenses/gpl-3.0.html
 */

#define LINE_1 0
#define LINE_2 1
#define FC_FILE_NAME "/fcards.txt"
#define MAX_LINE_LEN 255
char fcBuffer[MAX_LINE_LEN + 1]; // room for \0
const int  FC_DELAY = 5 * 1000;

struct FlashCard {
  String Tagalog;
  String English;
};

FlashCard currentCard;

// LCD constants
const int colorR = 0;
const int colorG = 0;
const int colorB = 255;
const int SCREEN_ROWS = 2;
const int SCREEN_COLS = 16;
const String TRUNC_CHARS = "...";

rgb_lcd lcd;

File fcFile;

const bool is_debug = false;

// Lines are 1 based
void printScreen(const char* line, bool cls = true, int whichLine = 1) {
  // Clear screen
  if(cls) {
    lcd.clear();
  }

 lcd.setCursor(0,whichLine - 1  );
 lcd.print(line);
}

void debugOut(String value, bool addLF = true) {
  if(is_debug) {
    if(addLF) {
      Serial.println(value);
    } else {
      Serial.print(value);
    }
  }
}

void setupLCD() {
  // set up the LCD's number of columns and rows:
  lcd.begin(SCREEN_COLS, SCREEN_ROWS);
  
  lcd.setRGB(colorR, colorG, colorB);
  
  // Print a message to the LCD.
  displayString("FlashCards by Tom Dison");
}

bool setupSDCard() {
    debugOut("\nInitializing SD card...", false);


  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  if (!SD.begin(BUILTIN_SDCARD)) {
    debugOut("initialization failed. Things to check:");
    debugOut("* is a card inserted?");
    debugOut("* is your wiring correct?");
    debugOut("* did you change the chipSelect pin to match your shield or module?");
    return false;
  } else {
   debugOut("Wiring is correct and a card is present.");
  }

  return true;
}

void setup()
{
 // Open serial communications and wait for port to open:
  if(is_debug) {
    Serial.begin(9600);
     
    while (!Serial) {
      ; // wait for serial port to connect.
    }
  }

  setupLCD();
  
  if(!setupSDCard()) {
    printScreen("Failed to access SD card!", true);
    debugOut("Failed to access SD card!");
    while(1); // don't continue
  }

  fcFile = SD.open(FC_FILE_NAME, FILE_READ);

  if(!fcFile) {
    printScreen("Could not open flash card file!");
    debugOut("Failed to access SD card!");
    while(1);
  }

  fcFile.seek(0);
}

bool isLineTerminator(char c) {
  return '\r' == c || '\n' == c;
}

String readLine() {
  int charsRead = 0;
  if(!fcFile.available()) {
    fcFile.seek(0);
  }
  while(!isLineTerminator(fcFile.peek())
  && charsRead <= MAX_LINE_LEN) {
     fcBuffer[charsRead] = fcFile.read();
     charsRead++; // Yes could be done in one line
  }

  fcBuffer[charsRead] = '\0';

  // Now remove line terminator(s)
  while(isLineTerminator(fcFile.peek())) {
    fcFile.read(); // discard
    debugOut("Discarded terminator");
  }
  
  return String(fcBuffer);
}

void readNextCard() {
  currentCard.Tagalog = "";
  currentCard.English = "";

  currentCard.Tagalog = readLine();;
  currentCard.English = readLine();
}

int getNextChunkPos(int startPos, String value, int max_chars,
    bool truncate) {
  int spacePos = 0;
  int lastPos = startPos + max_chars;
  
  if(lastPos > value.length()) {
    lastPos = value.length();
  } else {
    spacePos = value.lastIndexOf(' ', lastPos);
    if(spacePos != -1 && spacePos > startPos) {
      lastPos = spacePos;
    }

    if(truncate && 
        ((lastPos - startPos) > (max_chars - TRUNC_CHARS.length()))) {
      // We need to allow room for ...
      spacePos = value.lastIndexOf(' ', lastPos - 1);
      
      if(spacePos != -1 && spacePos > startPos) {
        lastPos = spacePos; 
      }
    }
  }

  return lastPos;
}

void displayString(String value) {
  int startPos = 0;
  int curLine = 1;
  int lastPos = 0;
  bool moreChunks = true;
  String line = "";

  while(moreChunks) {
    lastPos = getNextChunkPos(startPos, value, SCREEN_COLS, curLine == SCREEN_ROWS);

    if(lastPos == -1) {
      moreChunks = false;
    } else {
      moreChunks = lastPos < value.length();

      if(curLine == SCREEN_ROWS && moreChunks) {
        line = value.substring(startPos, lastPos) + TRUNC_CHARS;
      } else {
        line = value.substring(startPos, lastPos);
      }

      printScreen(line.c_str(), curLine == 1, curLine);
  
      if(moreChunks) {
        startPos = lastPos;
        
        if(value.charAt(startPos) == ' ') {
          startPos++;
        }
        
        if(++curLine == SCREEN_ROWS + 1) {
          curLine = 1;
          delay(FC_DELAY); // Leave this part of the verse up
        }
      } else {
          delay(FC_DELAY); // we are done with verse
      }
    }
  }
}

bool displayCard() {
  if(currentCard.Tagalog.length() == 0 ||
      currentCard.English.length() == 0) 
  {
    debugOut("Empty Flash Card!");
    return false;
  }

  debugOut("Tagalog: ", false);
  debugOut(currentCard.Tagalog, false);
  debugOut(", English: ", false);
  debugOut(currentCard.English);

  debugOut("Display Tagalog");
  displayString(currentCard.Tagalog.c_str());

  debugOut("Display English");
  displayString(currentCard.English.c_str());;

  return true;
}

bool showNextCard() {
  debugOut("Reading next Card");
  readNextCard();

  bool result = displayCard();
  debugOut("Result of display card is: ", false);
  debugOut(result); 
  return result;
}

void loop(void) {
  if(!showNextCard()) {
    printScreen("Error showing flash cards!");
    debugOut("Error showing flash cards");
    while(1);    
  }
}
