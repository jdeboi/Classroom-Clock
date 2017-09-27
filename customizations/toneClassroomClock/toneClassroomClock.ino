/////////////////////////////////////////////////////////
/*
  Jenna deBoisblanc
  2016
  http://jdeboi.com/

  CLASSROOM CLOCK with tone()
  A customization to the CLASSROOM CLOCK: adding a sound
  when the countdown timer is triggered.

*/
/////////////////////////////////////////////////////////

#include "pitches.h"

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
#define SPEED_CLOCK true

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
RTC_DS1307 RTC;
DateTime now;
uint8_t currentPeriod = 0;
uint8_t lastHour = 0;
uint8_t timeBlockIndex = 0;

DateTime lastFlash;    // for countdown "flash"
boolean flashOn = false;
long lastSpeedTest = 0;
boolean playedTone = false;

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
   Set the functionality of the extra digit
   show nothing in the extra digit is mode 0
   show rotating block is mode 1
   show period number is mode 2
*/
int extraDigitMode = 1;

// setup for a rotating block schedule
uint8_t currentBlock = A_BLOCK;

// this number should match the number of entries in schedule[]
const uint8_t numTimeBlocks = 9;

uint8_t schedule[numTimeBlocks][5] = {
  // use 24 hour clock numbers even though this clock is a 12 hour clock
  // {starting hour, start min, end hour, end min}
  {8, 0, 9, 0, ACADEMIC},       // 0 - Period 1: 8 - 9
  {9, 3, 9, 48, ACADEMIC},      // 1 - Period 2: 9:03 - 9:48
  {9, 48, 10, 15, ASSEMBLY},    // 2 - Assembly: 9:48 - 10:18
  {10, 18, 11, 3, ACADEMIC},    // 3 - Period 3: 10:18 - 11:03
  {11, 6, 11, 51, ACADEMIC},    // 4 - Period 4: 11:06 - 11:51
  {11, 54, 12, 39, ACADEMIC},   // 5 - Period 5: 11:54 - 12:39
  {12, 39, 13, 24, LUNCH},      // 6 - Lunch: 12:39 - 1:24
  {13, 27, 14, 12, ACADEMIC},   // 7 - Period 6: 1:27 - 2:12
  {14, 15, 15, 0, ACADEMIC}     // 8 - Period 7: 2:15 - 3:00
};

// this number should match the number of entries in vacations[]
const uint8_t numVacations = 3;
uint8_t vacations[numVacations][3] = {
  {2016, 1, 10},
  {2016, 2, 20},
  {2016, 3, 30}
};

// these are the array indicies of non-academic time blocks
const uint8_t assemblyBlock = 2;
const uint8_t lunchBlock = 6;

// number of minutes before end of class when countdown clock is triggered
uint8_t countdownM = 6;
uint8_t secBetweenFlashes = 4;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// THE GUTS
/////////////////////////////////////////////////////////
void setup() {
  Serial.begin(57600);
  /*
     For testing purposes, you can set the clock to custom values, e.g.:
     initChronoDot(year, month, day, hour, minute, seconds);
     Otherwise, the clock automatically sets itself to your computer's
     time with the function initChronoDot();
  */
  //initChronoDot(2016, 11, 7, 8, 54, 50);
  initChronoDot();
  strip.begin();
  strip.show();
  setSchedule();
  delay(3000);
}

void loop() {
  now = RTC.now();
  if (isEndOfDay()) nextDay();
  checkBlock();
  displayClock();
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// CUSTOMIZE DISPLAY FUNCTIONALITY
/////////////////////////////////////////////////////////

void displayClock() {
  if (!isSchoolDay()) colorClock(Wheel(0));
  else if (isBeforeSchool()) pulseClock(Wheel(40), 5);
  else if (isAfterSchool()) colorClock(Wheel(100));
  else if (isEndFlash()) countdownClock();
  else if (isLunch()) birthdayClock(300);
  else if (isAssembly()) rainbowClock(5);
  else if (isDuringClass()) gradientClock();
  else {
    //between classes
    mardiGrasClock();
  }
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// CHECK
/////////////////////////////////////////////////////////
void checkBlock() {
  if (timeBlockIndex < numTimeBlocks) {
    uint8_t h = schedule[timeBlockIndex][2];
    uint8_t m = schedule[timeBlockIndex][3];
    if (isAfterTime(now.hour(), now.minute(), h, m)) {
      if (schedule[timeBlockIndex][4] == ACADEMIC) {
        currentPeriod++;
        currentBlock++;
        if (currentBlock == 8) currentBlock = 0;
        if (DEBUG) {
          Serial.print("Academic period changed to: ");
          Serial.print(currentPeriod);
          Serial.print(" & block changed to: ");
          Serial.println(currentBlock);
        }
      }
      timeBlockIndex++;
      if (DEBUG) {
        Serial.print("Time block index: ");
        Serial.println(timeBlockIndex);
      }
    }
  }
}

int getCurrentPeriod() {
  int period = -1;
  if (isBeforeSchool() || isAfterSchool()) period = 0;
  // check if it's before the end of an academic period
  // if it's not during an academic period, find the next academic period
  for (int i = 0; i < numTimeBlocks; i++ ) {
    if (schedule[i][4] == ACADEMIC) {
      period++;
    }
    if (isBeforeTime(now.hour(), now.minute(), schedule[i][2], schedule[i][3])) {
      break;
    }
  }
  return period;
}

int getCurrentTimeBlock() {
  int tb = 0;
  if (isBeforeSchool() || isAfterSchool()) tb = 0;
  // check if it's before the end of an academic period
  // if it's not during an academic period, find the next academic period
  for (int i = 0; i < numTimeBlocks; i++ ) {
    if (isBeforeTime(now.hour(), now.minute(), schedule[i][2], schedule[i][3])) {
      return i;
    }
  }
  return tb;
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

boolean isAfterTime(uint8_t h0, uint8_t m0, uint8_t s0, uint8_t h1, uint8_t m1, uint8_t s1) {
  if (timeDiff(h0, m0, s0, h1, m1, s0) >= 0) return true;
  return false;
}

// returns the difference in minutes
// later time first
int timeDiff(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  uint16_t t0 = h0 * 60 + m0;
  uint16_t t1 = h1 * 60 + m1;
  return t0 - t1;
}

// returns the difference in seconds
// later time first
int timeDiff(uint8_t h0, uint8_t m0, uint8_t s0, uint8_t h1, uint8_t m1, uint8_t s1) {
  uint16_t t0 = h0 * 60 * 60 + m0 * 60 + s0;
  uint16_t t1 = h1 * 60 * 60 + m1 * 60 + s1;
  return t0 - t1;
}

boolean isEndFlash() {
  uint8_t h;
  uint8_t m;
  if (isDuringTimeBlocks()) {
    h = schedule[timeBlockIndex][2];
    m = schedule[timeBlockIndex][3];
  }
  else if (isBetweenTimeBlocks()) {
    h = schedule[timeBlockIndex][0];
    m = schedule[timeBlockIndex][1];
  }
  else {
    return false;
  }
  if (timeDiff(h, m, now.hour(), now.minute())  < countdownM) {
    if (!playedTone) {
      Serial.println("tone!");
      alarm();
      playedTone = true;
    }
    if (DEBUG) {
      Serial.print("End Flash: ");
      Serial.print(timeDiff( h, m, now.hour(), now.minute()));
      Serial.println(" minutes remaining");
    }
    if (now.unixtime() - lastFlash.unixtime() > secBetweenFlashes) {
      lastFlash = now;
      flashOn = !flashOn;
    }
    return flashOn;
  }
  playedTone = false;
  return false;
}

boolean isSchoolDay() {
  // 0 = Sunday, 1 = Monday, ...., 6 = Saturday
  if (now.dayOfWeek() == 0 || now.dayOfWeek() == 6) {
    if (DEBUG) Serial.println("Weekend!");
    return false;
  }
  else if (isVacation()) return false;
  return true;
}

boolean isVacation() {
  for (int i = 0; i < numVacations; i++) {
    if (now.year() == vacations[i][0] && now.month() == vacations[i][1] && now.day() == vacations[i][2]) {
      return true;
    }
  }
  return false;
}

boolean isBetweenTime(uint8_t h0, uint8_t m0, uint8_t h1, uint8_t m1) {
  DateTime startTime (now.year(), now.month(), now.day(), h0, m0, 0);
  DateTime endTime (now.year(), now.month(), now.day(), h1, m1, 0);
  return (now.unixtime() >= startTime.unixtime() && now.unixtime() < endTime.unixtime());
}

boolean isAssembly() {
  if (isBetweenTime(schedule[assemblyBlock][0], schedule[assemblyBlock][1], schedule[assemblyBlock][2], schedule[assemblyBlock][3])) {
    if (DEBUG) Serial.println("Assembly!");
    return true;
  }
  return false;
}

boolean isLunch() {
  if (isBetweenTime(schedule[lunchBlock][0], schedule[lunchBlock][1], schedule[lunchBlock][2], schedule[lunchBlock][3])) {
    if (DEBUG) Serial.println("Lunch!");
    return true;
  }
  return false;
}

boolean isAfterSchool() {
  // You may need to edit these values if school ends with a homeroom or
  // otherBlock[] that isn't a classperiod[]
  if (now.hour() >= schedule[numTimeBlocks - 1][2] && now.minute() >=  schedule[numTimeBlocks - 1][3]) {
    if (DEBUG) Serial.println("After School!");
    return true;
  }
  return false;
}

boolean isBeforeSchool() {
  // You may need to edit these values if school begins with a homeroom or
  // otherBlock[] that isn't a classperiod[]
  if (now.hour() <= schedule[0][0] && now.minute() < schedule[0][1]) {
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

boolean isBetweenTimeBlocks() {
  return isDuringSchoolDay() && !isDuringTimeBlocks();
}

boolean isHourChange() {
  if (now.minute() == 0 && now.second() < 15) return true;
  return false;
}

boolean isDuringSchoolDay() {
  return isSchoolDay() && !isAfterSchool() && !isBeforeSchool();
}

boolean isDuringClass() {
  for (int i = 0; i < numTimeBlocks; i++ ) {
    if (isBetweenTime(schedule[i][0], schedule[i][1], schedule[i][2], schedule[i][3])) {
      if (schedule[i][4] == ACADEMIC) {
        if (DEBUG) Serial.println("During class!");
        return true;
      }
    }
  }
  return false;
}

boolean isDuringTimeBlocks() {
  for (int i = 0; i < numTimeBlocks; i++ ) {
    if (isBetweenTime(schedule[i][0], schedule[i][1], schedule[i][2], schedule[i][3])) {
      if (DEBUG) Serial.println("During a time block!");
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
  uint8_t h;
  uint8_t m;
  if (isDuringTimeBlocks()) {
    h = schedule[timeBlockIndex][2];
    m = schedule[timeBlockIndex][3];
  }
  else if (isBetweenTimeBlocks()) {
    h = schedule[timeBlockIndex][0];
    m = schedule[timeBlockIndex][1];
  }
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
  displayLetter(getLetter(), Wheel(random(0, 255)));
  displayColon(Wheel(random(0, 255)));
  unsigned long t = millis();
  strip.show();
  while (millis() - t < delayTime) displayColon(Wheel(random(0, 255)));
}

void xmasClock() {
  displayHour(now.hour(), Wheel(0));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(getLetter(), Wheel(0));
  displayColon(Wheel(0));
  strip.show();
}

void mardiGrasClock() {
  displayHour(now.hour(), Wheel(200));
  displayMinute(now.minute(), Wheel(80));
  displayLetter(getLetter(), Wheel(50));
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
  uint8_t h0 = schedule[timeBlockIndex][0];
  uint8_t m0 = schedule[timeBlockIndex][1];
  uint8_t h1 = schedule[timeBlockIndex][2];
  uint8_t m1 = schedule[timeBlockIndex][3];
  DateTime startTime(now.year(), now.month(), now.day(), h0, m0, 0);
  DateTime endTime(now.year(), now.month(), now.day(), h1, m1, 0);
  if (DEBUG) {
    Serial.print("Gradient Clock: ");
    Serial.print(map(now.unixtime(), startTime.unixtime(), endTime.unixtime(), 0, 100));
    Serial.println("% through period");
  }
  return map(now.unixtime(), startTime.unixtime(), endTime.unixtime(), 80, 0);
}

void displayHour(uint8_t h, uint32_t col) {
  if (h == 0) h = 12;
  else if (h > 12) h -= 12;
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
  if (DEBUG) {
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running");
    }
  }
  // This section grabs the current datetime and compares it to
  // the compilation time.  If necessary, the RTC is updated.
  now = RTC.now();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  if (now.unixtime() < compiled.unixtime()) {
    if (DEBUG) Serial.println("RTC is older than compile time! Updating");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  now = RTC.now();
  if (DEBUG) Serial.println("Setup complete.");
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
  if (DEBUG) Serial.println("Setup complete.");
}

uint8_t getLetter() {
  if (extraDigitMode == 0) return 9;
  else if (!isSchoolDay()) return 9;
  else if (isAfterSchool()) return 9;
  else if (extraDigitMode == 2) return currentPeriod + 1;
  else return currentBlock;
}

void setSchedule() {
  currentPeriod = getCurrentPeriod();
  timeBlockIndex = getCurrentTimeBlock();
  if (DEBUG) {
    Serial.print("Current (starting) period is set to: ");
    Serial.println(currentPeriod);
    Serial.print("Current (starting) time block index is set to: ");
    Serial.println(timeBlockIndex);
  }

}


void nextDay() {
  if (isSchoolDay()) {
    currentPeriod = 0;
    timeBlockIndex = 0;
  }
}

void printClock() {
  if (DEBUG) {
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  }
}

/*
 * Pass a time that you'd like the alarm to go off
 * as well as the amount of time (alarmSeconds) that you'd like it to go off
 */
void checkCustomAlarm(int hr, int mn, int sec, int alarmSeconds) {
  DateTime startAlarm (now.year(), now.month(), now.day(), hr, mn, sec);
  if (now.unixtime() > startAlarm.unixtime() && now.unixtime() < startAlarm.unixtime() + alarmSeconds) {
    alarm(); 
  }
}

void alarm() {
  int melody [] = {NOTE_G6, NOTE_B6, NOTE_A6, NOTE_G6, NOTE_B5};
  for (int i = 0; i < 5; i++) {
    tone(8, melody[i], 500);
    delay(800);
  }
  noTone(8);
}

