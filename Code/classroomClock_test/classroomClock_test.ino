/////////////////////////////////////////////////////////
/*
  Jenna deBoisblanc
  January 2016
  http://jdeboi.com/

  CLASSROOM CLOCK
  This customizable Arduino clock was developed for the Isidore
  Newman School Makerspace. Some of the features include:
  - tracks and displays Newman's rotating block (A, B, C...)
  - uses a red->green color gradient to show the amount of time
    remaining in the period
  - flashes between the time and a countdown timer when the end
    of the period approaches
  - rainbows during lunch and Assembly, pulses after school...
  And so much more! Add your own functions to make School Clock
  even cooler!

*/
/////////////////////////////////////////////////////////

#include "classroomClock.h"
#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"               // https://github.com/adafruit/RTClib
#include <RTC_DS1307.h>
#include <avr/power.h>
#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#define PIN 3
#define NUM_PIXELS 32

/////////////////////////////////////////////////////////
// SET THIS
int currentBlock = A_BLOCK;
/////////////////////////////////////////////////////////

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 RTC;
DateTime now;
uint8_t currentPeriod = 0;
uint8_t lastHour = 0;
DateTime lastFlash;    // for countdown "flash"
boolean flashOn = false;
const byte numVacations = 9;
uint16_t vacations[numVacations][3] = {
  {2016, 9, 7}, 
  {2016, 9, 14}, 
  {2016, 9, 23}
};

uint8_t periods[7][4] = {
  {8, 0, 9, 0},   // Period 0: 8 - 9
  {9, 3, 9, 48},  // Period 1: 9:03 - 9:48
  // Assembly: 9:48 - 10:18
  {10, 18, 11, 3}, // Period 2: 10:18 - 11:03
  {11, 6, 11, 51}, // Period 3: 11:06 - 11:51
  {11, 54, 12, 39}, // Period 4: 11:54 - 12:39
  // Lunch: 12:39 - 1:24
  {13, 27, 14, 12}, // Period 5: 1:27 - 2:12
  {14, 15, 15, 0} // Period 6: 2:15 - 3:00
};

byte numbers[] = {
  B11101110,    // 0
  B10001000,    // 1
  B01111100,    // 2     5555
  B11011100,    // 3   6     4
  B10011010,    // 4     3333
  B11010110,    // 5   2     0
  B11110110,    // 6     1111
  B10001100,    // 7
  B11111110,    // 8
  B10011110    // 9
};

byte letters[] = {
  B10111110,    // A      55555
  B11110010,    // B     6     4
  B01110000,    // C     6     4
  B11111000,    // D      33333
  B01110110,    // E     2     0
  B00110110,    // F     2     0
  B11011110,    // G      11111
  B10111010,    // H
  B01100010,    // L
  B00000000     //
};


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// THE GUTS
/////////////////////////////////////////////////////////
void setup() {
  Serial.begin(57600);
  initChronoDot();
  strip.begin();
  strip.show();
  delay(5000);
  now = RTC.now();
}

void loop() {
  
  for(int i = 0; i < 6*60; i++) {
    if(i%60 < 30) { 
      countdownClock(8*60*60+54*60+i,9*60*60);
    }
    else colorClock(8,54+i/60,0,Wheel(50));
    delay(100);
  }
  
  //showBlocks();
  //rainbowClock(10);
  //gradientClock();
  //birthdayClock(200);
  //xmasClock();
  //mardiGrasClock();
  //pulseClock(4,25,Wheel(100),5);
  //rainClock();
}



/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// GET/SET
/////////////////////////////////////////////////////////
// credit: Adafruit
void initChronoDot() {
  // Instantiate the RTC
  Wire.begin();
  RTC.begin();
 
  // Check if the RTC is running.
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running");
  }

  // This section grabs the current datetime and compares it to
  // the compilation time.  If necessary, the RTC is updated.
  DateTime now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime()) {
    Serial.println("RTC is older than compile time! Updating");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  now = RTC.now();
  Serial.println("Setup complete.");
}




/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// DISPLAY
/////////////////////////////////////////////////////////

void showBlocks() {
  int i = 0;
  long lastTime = millis();
  while (i < 10) {
    displayHour(now.hour(), Wheel(240));
    displayMinute(now.minute(), Wheel(240));
    displayLetter(i, Wheel(180));
    displayColon(Wheel(240));
    if(millis() > lastTime+500) {
      i++;
      lastTime = millis();
    }
    strip.show();
  }
}

void displayColon(uint32_t c) {
  now = RTC.now();
  if (now.second() % 2 == 0) {
    Serial.print("r=0");
    Serial.println(now.second());
    strip.setPixelColor(21, c);
    strip.setPixelColor(22, c);
  }
  else {
    Serial.print("r=1");
    Serial.println(now.second());
    strip.setPixelColor(21, 0);
    strip.setPixelColor(22, 0);
  }
}

void countdownClock(long currentTime, long endTime) {
  uint8_t minLeft = (endTime-currentTime)/60;
  uint8_t secLeft = (endTime - currentTime)%60;

  if(minLeft == 0) displayHour(0, 0);
  else displayHour(minLeft, Wheel(190));
  displayMinute(secLeft, Wheel(190));
  displayLetter(1, Wheel(190));
  displayColon(Wheel(190));
  strip.show();
}

void colorClock(int h, int m, int l, int c) {
  displayHour(h, Wheel(c));
  displayMinute(m, Wheel(c));
  displayLetter(l, Wheel(c));
  displayColon(Wheel(c));
  strip.show();
}

void rainbowClock(int delayTime) {
  for (int j = 0; j < 256; j++) {
    displayHour(12, Wheel(j));
    displayMinute(15, Wheel(j));
    displayLetter(10, Wheel(j));
    displayColon(Wheel(j));
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      displayColon(Wheel(j));
      strip.show();
    }
  }
}

void birthdayClock(int delayTime) {
  displayHour(now.hour(), Wheel(random(0,255)));
  displayMinute(now.minute(), Wheel(random(0,255)));
  displayLetter(0, Wheel(random(0,255)));
  displayColon(Wheel(random(0,255)));
  unsigned long t = millis();
  strip.show();
  while(millis() - t < delayTime) displayColon(Wheel(random(0,255)));
}

void xmasClock() {
  displayHour(now.hour(), Wheel(0));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(1, Wheel(0));
  displayColon(Wheel(0));
  strip.show();
}

void rainClock() {
  if(now.second()&2 == 0) {
    displayHour(now.hour(), Wheel(0));
    displayMinute(now.minute(), Wheel(80));
    displayLetter(1, Wheel(100));
    displayColon(Wheel(50));
  }
  else {
    displayHour(now.hour(), 0);
    displayMinute(now.minute(), 0);
    displayLetter(1, 0);
    displayColon(0);
  }
  strip.show();
}

void mardiGrasClock() {
  displayHour(now.hour(), Wheel(200));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(0, Wheel(50));
  displayColon(Wheel(50));
  strip.show();
}

void pulseClock(int h, int m, uint32_t col, int delayTime) {
  for (int j = 255; j >= 0; j--) {
    now = RTC.now();
    displayHour(h, col);
    displayMinute(m, col);
    displayLetter(4, col);
    displayColon(col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      displayColon(col);
      strip.show();
    }
  }
  for (int j = 0; j < 256; j++) {
    displayHour(h, col);
    displayMinute(m, col);
    displayLetter(4, col);
    displayColon(col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      displayColon(col);
      strip.show();
    }
  }
}

void gradientClock() {
  int h = 8;
  for(int i = 0; i < 60; i++) {
    uint8_t t = map(h*60+i, 8*60, 9*60, 80, 0);
    uint32_t c = Wheel(t);
    displayHour(h, c);
    displayMinute(i, c);
    displayColon(c);
    displayLetter(4, c);
    strip.show();
    unsigned long tt = millis();
    while(millis() - tt < 100) {
      displayColon(c);
      strip.show();
    }
  }
  delay(3000);
}

void displayHour(uint8_t h, uint32_t col) {
  if(h > 12) h = h-12;
  uint8_t firstDigit = h / 10;
  uint8_t secondDigit = h % 10;
  // TODO - this only works for non-military time
  // firstDigit is either off or all on (0 or 1)
  if (firstDigit > 0) {
    strip.setPixelColor(30, col);
    strip.setPixelColor(31, col);
  }
  else {
    strip.setPixelColor(30, 0);
    strip.setPixelColor(31, 0);
  }
  // secondDigit
  for (int i = 0; i < 7; i++) {
    if (numbers[secondDigit] & (1 << 7 - i)) strip.setPixelColor(i + (7 * 3 + 2), col);
    else strip.setPixelColor(i + (7 * 3 + 2), 0);
  }
  if(now.minute()/10 == 2) strip.setPixelColor(14,0);
}

void displayMinute(uint8_t m, uint32_t col) {
  uint8_t firstDigit = m / 10;
  uint8_t secondDigit = m % 10;
  
  // first digit (first from left to right)
  for (int i = 0; i < 7; i++) {
    if (numbers[firstDigit] & (1 << 7 - i)) strip.setPixelColor(i + (7 * 2), col);
    else {
      strip.setPixelColor(i + (7 * 2), 0);
    }
  }
  // second digit (least significant)
  for (int i = 0; i < 7; i++) {
    if (numbers[secondDigit] & (1 << 7 - i)) {
      strip.setPixelColor(i + 7, col);
    }
    else {
      strip.setPixelColor(i + 7, 0);
    }
  }
}

void displayLetter(uint8_t letter, uint32_t col) {
  if (letter < 8) {
    for (int i = 0; i < 7; i++) {
      if (letters[letter] & (1 << 7 - i)) strip.setPixelColor(i, col);
      else strip.setPixelColor(i, 0);
    }
  }
  else {
    for (int i = 0; i < 7; i++) {
      strip.setPixelColor(i, 0);
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// credit - Adafruit
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}




