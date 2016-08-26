/////////////////////////////////////////////////////////
/*
  Jenna deBoisblanc
  2016
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

#define DEBUG false

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 RTC;
DateTime now;
uint8_t currentPeriod = 0;
uint8_t lastHour = 0;
DateTime lastFlash;    // for countdown "flash"
boolean flashOn = false;
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
// CUSTOMIZE THIS STUFF
/////////////////////////////////////////////////////////

/*
 * Set the functionality of the extra digit
 * show nothing in the extra digit is mode 0
 * show rotating block is mode 1
 * show period number is mode 2
*/
int extraDigitMode = 1;

// setup for a rotating block schedule
uint8_t currentBlock = D_BLOCK;

// this number should match the number of entries in classPeriods[]
const uint8_t numClassPeriods = 7;

uint8_t classPeriods[numClassPeriods][4] = {
  // use 24 hour clock numbers even though this clock is a 12 hour clock
  // {starting hour, start min, end hour, end min}
  {8, 0, 9, 0},       // Period 0: 8 - 9
  {9, 3, 9, 48},      // Period 1: 9:03 - 9:48
  // Assembly: 9:48 - 10:18
  {10, 18, 11, 3},    // Period 2: 10:18 - 11:03
  {11, 6, 11, 51},    // Period 3: 11:06 - 11:51
  {11, 54, 12, 39},   // Period 4: 11:54 - 12:39
  // Lunch: 12:39 - 1:24
  {13, 27, 14, 12},   // Period 5: 1:27 - 2:12
  {14, 15, 15, 0}     // Period 6: 2:15 - 3:00
};

// this number should match the number
const uint8_t numOtherBlocks = 2;   

// of entries in otherBlocks[]
uint8_t otherBlocks[numOtherBlocks][4] = {
  {9, 48, 10, 18},   // assembly: 9:48 - 10:18
  {12, 39, 13, 24}  // Lunch: 12:39 - 1:24
};

// the next two values probably don't need to be changed
// they represent their index in the otherBlocks array
const uint8_t assemblyBlock = 0;
const uint8_t lunchBlock = 1;

// number of minutes before end of class when countdown clock is triggered
uint8_t countdownM = 6;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// THE GUTS
/////////////////////////////////////////////////////////
void setup() {
  Serial.begin(57600);
  
  /* 
   * For testing purposes, you can set the clock to custom values, e.g.:
   * initChronoDot(year, month, day, hour, minute, seconds);
   * initChronoDot(2016, 7, 26, 11, 47, 50);
   * 
   * Otherwise, the clock automatically sets itself to your computer's
   * time with the function initChronoDot();
   * 
  */
  initChronoDot();
  strip.begin();
  strip.show();

  /* 
   *  Periods, unlike blocks, are only dependent on the time of
   *  day and increment from 0 to whatever the last period is.
   *  setPeriod() determines which period it currently is based on
   *  the current time. Blocks increment at the end of each period.
   */
  setPeriod();

  /*
   * Be careful about not drawing too much current from your USB.
   * Make sure there's an external power supply, and to be really 
   * careful, it might be a good idea to unplug after uploading code.
   * This delay is designed to give you time to upload and uplug
   * before Neopixels come on.
   */
  delay(5000);
}

void loop() {
  now = RTC.now();
  displayClock();
  checkBlock();
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// CUSTOMIZE DISPLAY FUNCTIONALITY
/////////////////////////////////////////////////////////

void displayClock() {
  if (isEndOfDay()) nextDay();
  else if (!isSchoolDay()) colorClock(Wheel(0));
  else if (isBeforeSchool()) colorClock(Wheel(40));
  else if (isAfterSchool()) colorClock(Wheel(100));
  else if (isEndFlash()) countdownClock();
  else if (isLunch()) colorClock(Wheel(150));
  else if (isAssembly()) rainbowClock(5);
  else if (isDuringClass()) gradientClock();
  else colorClock(Wheel(200));
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// CHECK
/////////////////////////////////////////////////////////
void checkBlock() {
  if (currentPeriod < numClassPeriods) {
    uint8_t h = classPeriods[currentPeriod][2];
    uint8_t m = classPeriods[currentPeriod][3];
    if (isAfterTime(now.hour(), now.minute(), h, m)) {
      currentPeriod++;
      currentBlock++;
      if (currentBlock == 8) currentBlock = 0;
      if (DEBUG) {
        Serial.print(" Period changed to: ");
        Serial.print(currentPeriod);
        Serial.print(" & Block changed to: ");
        Serial.println(currentBlock);
      }
    }
  }
  if (DEBUG) {
    Serial.print("Current block: ");
    Serial.println(currentBlock);
  }
}

// returns true if the first number is before the
boolean isBeforeTime(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  if (timeDiff(h0, m0, h1, m1) < 0) return true;
  return false;
}

boolean isAfterTime(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  if (timeDiff(h0, m0, h1, m1) >= 0) return true;
  return false;
}

// returns the difference in minutes
// later time first
int timeDiff(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  uint16_t t0 = h0 * 60 + m0;
  uint16_t t1 = h1 * 60 + m1;

  return t0 - t1;
}

boolean isEndFlash() {
  uint8_t h = classPeriods[currentPeriod][2];
  uint8_t m = classPeriods[currentPeriod][3];
  if (timeDiff(h, m, now.hour(), now.minute())  < countdownM) {
    if (DEBUG) {
      Serial.print("End Flash: ");
      Serial.print(timeDiff( h, m, now.hour(), now.minute()));
      Serial.println(" minutes remaining");
    }
    if (now.unixtime() - lastFlash.unixtime() > 4) {
      lastFlash = now;
      flashOn = !flashOn;
    }
    return flashOn;
  }
  return false;
}

boolean isSchoolDay() {
  // 0 = Sunday, 1 = Monday, ...., 6 = Saturday
  if (now.dayOfWeek() == 0 || now.dayOfWeek() == 6) {
    if (DEBUG) Serial.println("Weekend!");
    return false;
  }
  // else if (isVacation()) return false;
  return true;
}

//boolean isVacation() {
//  for (int i = 0; i < numVacations; i++) {
//    if(now.year() == vacations[i][0] && now.month() == vacations[i][1] && now.day() == vacations[i][2]) {
//      return true;
//    }
//  }
//  return false;
//}

boolean isBetweenTime(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  DateTime startTime (now.year(), now.month(), now.day(), h0, m0, 0);
  DateTime endTime (now.year(), now.month(), now.day(), h1, m1, 0);
  return (now.unixtime() >= startTime.unixtime() && now.unixtime() < endTime.unixtime());
}

boolean isAssembly() {
  if (isBetweenTime(otherBlocks[assemblyBlock][0], otherBlocks[assemblyBlock][1], otherBlocks[assemblyBlock][2], otherBlocks[assemblyBlock][2])) {
    if (DEBUG) Serial.println("Assembly!");
    return true;
  }
  return false;
}

boolean isLunch() {
  if (isBetweenTime(otherBlocks[lunchBlock][0], otherBlocks[lunchBlock][1], otherBlocks[lunchBlock][2], otherBlocks[lunchBlock][2])) {
    if (DEBUG) Serial.println("Lunch!");
    return true;
  }
  return false;
}

boolean isAfterSchool() {
  // You may need to edit these values if school ends with a homeroom or
  // otherBlock[] that isn't a classperiod[]
  if (now.hour() >= classPeriods[numClassPeriods - 1][2] && now.minute() >=  classPeriods[numClassPeriods - 1][3]) {
    if (DEBUG) Serial.println("After School!");
    return true;
  }
  return false;
}

boolean isBeforeSchool() {
  // You may need to edit these values if school begins with a homeroom or
  // otherBlock[] that isn't a classperiod[]
  if (now.hour() <= classPeriods[0][0] && now.minute() < classPeriods[0][1]) {
    if (DEBUG) Serial.println("Before School!");
    return true;
  }
  return false;
}

boolean isEndOfDay() {
  boolean changed = false;
  if (now.hour() == 0 && lastHour == 23) {
    changed = true;
  }
  lastHour = now.hour();
  return changed;
}


boolean isHourChange() {
  if (now.minute() == 0 && now.second() < 15) return true;
  return false;
}

boolean isEnd() {
  uint8_t h = classPeriods[currentPeriod][2];
  uint8_t m = classPeriods[currentPeriod][3];
  DateTime endTime (now.year(), now.month(), now.day(), h, m, 0);
  if (endTime.unixtime() - now.unixtime()  < 7 * 60) return true;
  return false;
}

boolean isDuringClass() {
  for (int i = 0; i < numClassPeriods; i++ ) {
    if (isBetweenTime(classPeriods[i][0], classPeriods[i][1], classPeriods[i][2], classPeriods[i][3])) {
      if (DEBUG) Serial.println("During Class!");
      return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// DISPLAY
/////////////////////////////////////////////////////////

void displayColon(uint32_t c) {
  if (now.second() % 2 == 0) {
    strip.setPixelColor(21, c);
    strip.setPixelColor(22, c);
  }
  else {
    strip.setPixelColor(21, 0);
    strip.setPixelColor(22, 0);
  }
}

void countdownClock() {
  uint8_t h = classPeriods[currentPeriod][2];
  uint8_t m = classPeriods[currentPeriod][3];
  DateTime endTime(now.year(), now.month(), now.day(), h, m, 0);
  uint8_t minLeft = (endTime.unixtime() - now.unixtime()) / 60;
  uint8_t secLeft = (endTime.unixtime() - now.unixtime()) - minLeft * 60;
  if (minLeft == 0) displayHour(minLeft, 0);
  else displayHour(minLeft, Wheel(190));
  displayMinute(secLeft, Wheel(190));
  displayLetter(getLetter(), Wheel(190));
  displayColon(Wheel(190));
  strip.show();
}

void colorClock(int c) {
  displayHour(now.hour(), Wheel(c));
  displayMinute(now.minute(), Wheel(c));
  displayLetter(getLetter(), Wheel(c));
  displayColon(Wheel(c));
  strip.show();
}

void rainbowClock(int delayTime) {
  for (int j = 0; j < 256; j++) {
    displayHour(now.hour(), Wheel(j));
    displayMinute(now.minute(), Wheel(j));
    displayLetter(getLetter(), Wheel(j));
    displayColon(Wheel(j));
    strip.show();
    unsigned long t = millis();
    while (millis() - t < delayTime) {
      displayColon(Wheel(j));
      strip.show();
    }
  }
}

void birthdayClock(int delayTime) {
  displayHour(now.hour(), Wheel(random(0, 255)));
  displayMinute(now.minute(), Wheel(random(0, 255)));
  displayLetter(0, Wheel(random(0, 255)));
  displayColon(Wheel(random(0, 255)));
  unsigned long t = millis();
  strip.show();
  while (millis() - t < delayTime) displayColon(Wheel(random(0, 255)));
}

void xmasClock() {
  displayHour(now.hour(), Wheel(0));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(1, Wheel(0));
  displayColon(Wheel(0));
  strip.show();
}

void mardiGrasClock() {
  displayHour(now.hour(), Wheel(200));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(0, Wheel(50));
  displayColon(Wheel(50));
  strip.show();
}

void randoClock(int delayTime) {
  displayHour(now.hour(), Wheel(random(0, 255)));
  displayMinute(now.minute(), Wheel(random(0, 255)));
  displayLetter(getLetter(), Wheel(random(0, 255)));
  displayColon(Wheel(random(0, 255)));
  unsigned long t = millis();
  while (millis() - t < delayTime) displayColon(Wheel(random(0, 255)));
}

void pulseClock(uint32_t col, int delayTime) {
  for (int j = 255; j >= 0; j--) {
    displayHour(now.hour(), col);
    displayMinute(now.minute(), col);
    displayLetter(getLetter(), col);
    displayColon(col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while (millis() - t < delayTime) {
      displayColon(col);
      strip.show();
    }
  }
  for (int j = 0; j < 256; j++) {
    displayHour(now.hour(), col);
    displayMinute(now.minute(), col);
    displayLetter(getLetter(), col);
    displayColon(col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while (millis() - t < delayTime) {
      displayColon(col);
      strip.show();
    }
  }
}

void gradientClock() {
  uint32_t c = Wheel(getGradientColor(now.hour(), now.minute()));
  displayHour(now.hour(), c);
  displayMinute(now.minute(), c);
  displayColon(c);
  displayLetter(getLetter(), c);
  strip.show();
}

int getGradientColor(uint8_t h, uint8_t m) {
  // DateTime (year, month, day, hour, min, sec);
  uint8_t h0 = classPeriods[currentPeriod][0];
  uint8_t m0 = classPeriods[currentPeriod][1];
  uint8_t h1 = classPeriods[currentPeriod][2];
  uint8_t m1 = classPeriods[currentPeriod][3];
  DateTime startTime(now.year(), now.month(), now.day(), h0, m0, 0);
  DateTime endTime(now.year(), now.month(), now.day(), h1, m1, 0);
  // not working if(now.unixtime() < startTime.unixtime()) return 200;
  // not working else return map(now.unixtime(), startTime.unixtime(), endTime.unixtime(), 80, 0);
  map(now.unixtime(), startTime.unixtime(), endTime.unixtime(), 80, 0);
}

void displayHour(uint8_t h, uint32_t col) {
  if (h > 12) h = h - 12;
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
  if (now.minute() / 10 == 2) strip.setPixelColor(14, 0);
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

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// GET/SET
/////////////////////////////////////////////////////////
// credit: Adafruit
// Use this function to automatically set an initial DateTime
// using time from the computer/ compiler
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
  now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime()) {
    Serial.println("RTC is older than compile time! Updating");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  now = RTC.now();
  Serial.println("Setup complete.");
}

// Use this function to set a specific initial DateTime
// useful for debugging
void initChronoDot(int y, int mon, int d, int h, int minu, int s) {
  // Instantiate the RTC
  Wire.begin();
  RTC.begin();
  // Check if the RTC is running.
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running");
  }
  now = RTC.now();
  RTC.adjust(DateTime(y, mon, d, h, minu, s));
  now = RTC.now();
  Serial.println("Setup complete.");
}

uint8_t getLetter() {
  if (extraDigitMode == 0) return 10;
  else if (isAfterSchool()) return 10;
  else if (!isSchoolDay()) return 10;
  else if (extraDigitMode == 2) return currentPeriod + 1;
  else return currentBlock;
}

void setPeriod() {
  currentPeriod = 0;
  for (int i = 0; i < numClassPeriods; i++ ) {
    if (isBetweenTime(classPeriods[i][0], classPeriods[i][1], classPeriods[i][2], classPeriods[i][3])) {
      currentPeriod = i;
      break;
    }
  }
  if (DEBUG) {
    Serial.print("Current period (#): ");
    Serial.println(currentPeriod);
  }
}

void nextDay() {
  if (isSchoolDay()) {
    currentPeriod = 0;
    currentBlock++;
    if (currentBlock == 8) currentBlock = 0;
  }
}



void printClock() {
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
}

