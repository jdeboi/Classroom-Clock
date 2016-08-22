/////////////////////////////////////////////////////////
/*
  Jenna deBoisblanc
  January 2016
  http://jdeboi.com/

*/
/////////////////////////////////////////////////////////

#include <Wire.h>
#include <SPI.h>
#include "RTClib.h"               // https://github.com/adafruit/RTClib
#include <RTC_DS1307.h>
#include "classroomClock.h"
#include <avr/power.h>
#include <Adafruit_NeoPixel.h>    // https://github.com/adafruit/Adafruit_NeoPixel

#define PIN 2
#define NUM_PIXELS 32

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);



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

DateTime currentTime, endTime;
int digits[5];
unsigned long lastCheck;
unsigned long hours;
unsigned long remainingDays;
int hourSet;
int minSet;
int letSet;
float fracDays;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// THE GUTS
/////////////////////////////////////////////////////////
void setup() {
  Serial.begin(57600);
  strip.begin();
  strip.show();

  currentTime = DateTime(__DATE__, __TIME__);
  endTime = DateTime(2017, MAY, 31, 19, 0, 0);

  setDifference();
 
}

void loop() {
  checkTime();
  setDifference();
  
  displayClock();
}



void checkTime() {
  if (millis() > lastCheck) {
    if (millis() - lastCheck > 1000 * 60 * 60) {
      currentTime = DateTime(currentTime.unixtime()+60*60);
      lastCheck = millis();
    }
  }
  else {
    if (4294967295 - lastCheck + millis() > 1000 * 60 * 60) {
       lastCheck = millis();
       currentTime = DateTime(currentTime.unixtime()+60*60);
    }
    
  }
}

void setDifference() {
  remainingDays = (endTime.unixtime() - currentTime.unixtime())/60/60/24;
  fracDays = int(((endTime.unixtime() - currentTime.unixtime()) - remainingDays)/(24.0*60*60)*10);
  hourSet = remainingDays/10; 
  if (hourSet > 19) hourSet = 12;
  minSet = remainingDays - hourSet*10;
  letSet = fracDays;
  Serial.println(minSet);
}



/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// DISPLAY
/////////////////////////////////////////////////////////
void displayClock() {
  if (currentTime.hour() == 8) rainbowClock(5);
  else if (currentTime.hour() == 9) colorClock(Wheel(240));
  else if (currentTime.hour() == 10) rainbowClock(5);
  else if (currentTime.hour() == 11) colorClock(Wheel(200)); 
  else if (currentTime.hour() == 12) pulseClock(Wheel(100),5);
  else colorClock(Wheel(250));
}






void colorClock(int c) {
  displayHour(hourSet, Wheel(c));
  displayMinute(minSet, Wheel(c));
  //displayLetter(letSet, Wheel(c));
  strip.show();
}

void rainbowClock(int delayTime) {
  for (int j = 0; j < 256; j++) {
    displayHour(hourSet, Wheel(j));
    displayMinute(minSet, Wheel(j));
    displayLetter(letSet, Wheel(j));
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      strip.show();
    }
  }
}

void randoClock(int delayTime) {
  displayHour(hourSet, Wheel(random(0,255)));
  displayMinute(minSet, Wheel(random(0,255)));
  //displayLetter(letSet, Wheel(random(0,255)));
}

void pulseClock(uint32_t col, int delayTime) {
  for (int j = 255; j >= 0; j--) {
    displayHour(hourSet, col);
    displayMinute(minSet, col);
    //displayLetter(letSet, col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      strip.show();
    }
  }
  for (int j = 0; j < 256; j++) {
    displayHour(hourSet, col);
    displayMinute(minSet, col);
    //displayLetter(letSet, col);
    strip.setBrightness(j);
    strip.show();
    unsigned long t = millis();
    while(millis() - t < delayTime) {
      strip.show();
    }
  }
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
  if(minSet/10 == 2) strip.setPixelColor(14,0);
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




