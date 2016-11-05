/**
 * (c) 2016 John Bakker <me@johnbakker.name>
 * 
 * Hamster fitness measurement device
 * Copyright 2016 John Bakker

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define HAMSTERFITNESS_VERSION "0.1"

#define PIN 6
#define hallPin 0
#define standardDev 50

#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

// please select if you have a large wheel or a small wheel. 
#define largewheel 1
const int largeWheelRadiusInMM= 85;
const int smallWheelRadiusInMM= 55;
//you can modify what the distance is you want to set as a goal. it is in MM
const long healthyHamsterDistance= 9000000; // 9km in mm (What a healthy hamster runs every night)
const long dayLength  = 86400000; //reset actual day length;
//const long healthyHamsterDistance= 9000; // 900cm in mm for testing
//const long dayLength  = 60000;//reset every 60 seconds
const boolean enableSerial=true;


Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B00000000,
  B00111000, B00011100,
  B01000111, B11100010,
  B01000000, B00000010,
  B01010000, B00001010,
  B00100000, B00000100,
  B00101100, B00110100,
  B01000000, B00000010,
  B01000001, B10000010,
  B01000000, B00000010,
  B00100000, B00000100,
  B00011000, B00011000,
  B00011111, B11111000,
  B00011000, B00011000,
  B00000000, B00000000,
  B00000000, B00000000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


long lastDayStart =0;
long nextDayStart =0;
long displaycheckUpdate= 0;


const float pi = 3.1416;
int lastMeasurement=0;
int normalState = 0;
uint32_t color;
uint32_t red;
uint32_t orange;
uint32_t purple;
uint32_t blue;
uint32_t teal;
uint32_t green;
uint32_t colorsPerLight[8] = {0,0,0,0,0,0,0,0};

long last7Days[7] = {0,0,0,0,0,0,0};
byte eightAbove=0;
byte eightBelow=0;
boolean hasMagnet=false;

long ticks = 0;
long rotationDistance = 0;
long distancePerPercent=0;
boolean needsDisplayUpdate=false;
int timeloop=0;
int turnOffDisplayTimer=0;
boolean rollOver=false;
int turnOffDisplayTimeout=9000;
boolean displayOn=true;

/**
*/
void setup() {
  if(enableSerial)
  {
    Serial.begin(9600);
  }
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Hamster fitness v.");
  display.println(HAMSTERFITNESS_VERSION);
  display.drawBitmap(16, 16,  logo16_glcd_bmp, 16, 16, 1);
  
  display.drawBitmap(48, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(80, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(112, 16,  logo16_glcd_bmp, 16, 16, 1);
  display.display();
  
  delay(2000);

  
  findNormalState();
  eightAbove = (1024 - normalState)/8;
  eightBelow = normalState /8;
  if(largewheel==1)
  {
    rotationDistance = 2 * pi * largeWheelRadiusInMM;  
  }
  else
  {
    rotationDistance = 2 * pi * smallWheelRadiusInMM; 
  }
  
  distancePerPercent = healthyHamsterDistance / 100 ;

  nextDayStart = lastDayStart + dayLength;
  displayOn=true;
  
}
long distanceinMMTraveled()
{
  return rotationDistance*ticks;
}
void findNormalState(){
  int nr[8] = {0,0,0,0,0,0,0,0};
  byte p = 0;
  int total = 0;
  for(p=0;p<8;p++)
  {
    total+=analogRead(hallPin);
    //strip.setPixelColor(p,green); 
    //strip.show();
    delay(100);
  }
  //now figure out the avarage;
  for(p=0;p<8;p++)
  {
    total+=analogRead(hallPin);
    //strip.setPixelColor(p,0); 
    //strip.show();
    delay(100);
  }
  normalState = total/16;
  if(enableSerial)
  {
   Serial.println("Normal state");
   Serial.println(normalState); 
  }
  
}

void updateSerial()
{

  if(enableSerial)
  {
    Serial.print("Distance traveled in MM");
    Serial.println(distanceinMMTraveled());
    Serial.print("Distance per percent:");
    Serial.println(distancePerPercent);
  }
}

void measure()
{
  //
  int raw = analogRead(0);
  if(raw >lastMeasurement-standardDev && raw<lastMeasurement-standardDev)
  {
    //nothing to do!
    lastMeasurement= raw;
    return;
  }
  if((raw >normalState+standardDev) || (raw <normalState-standardDev))
  {
    if(!hasMagnet)
    {
       hasMagnet=true;
       ticks++; 
       
       needsDisplayUpdate=true;
    }  
  }
  else
  {
    hasMagnet=false;
  }
  
}

void measureLights()
{
  int raw = analogRead(0);
  if(raw >lastMeasurement-standardDev && raw<lastMeasurement-standardDev)
  {
    //nothing to do!
    lastMeasurement= raw;
    return;
  }
  byte p=0;
  lastMeasurement= raw;
  if(raw >normalState+standardDev)
  {
     color = green;
     p= (raw - normalState) /eightAbove;
  }
  else if(raw <normalState-standardDev)
  {
     color = red;
     p= (normalState - raw) /eightBelow;
  }
  else
  {
    return;
  }
  
}
void updateDisplay()
{
    if(!needsDisplayUpdate)
    {
       return;
    }
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Rotations:");
    display.println(ticks);
    //ticks
    needsDisplayUpdate=false;
    long distanceTraveled = distanceinMMTraveled();
    long width = distanceTraveled /distancePerPercent;
    
    display.print("Distance:");
    
    if(distanceTraveled >=1000000)
    {
      
      float traveledFL = distanceTraveled /1000000;

      display.print(traveledFL);
      display.println("KM");
    }else if(distanceTraveled >=1000)
    {
      
      
      display.print(distanceTraveled /1000);
      display.println("M");
    }else if(distanceTraveled >=10)
    {
      
      
      display.print(distanceTraveled /10);
      display.println("CM");
    }
    else 
    {
      display.print(distanceTraveled);
      display.println("MM");
    }
    
    
    
    
    
    
    
    if(width>100)
    {
      width=100;
    }
    display.drawLine(13, 39, 115, 39, WHITE); 
    display.drawLine(13, 39, 13, 44, WHITE); 
    display.drawLine(13, 45, 115, 45, WHITE); 
    display.drawLine(115, 39, 115, 44, WHITE); 
    for(byte h=0;h<5;h++)
    {
      display.drawLine(14, 40+h, 14+width, 40+h, WHITE);  
    }
    display.display();
    triggerDisplayOff();
    
}

void keepTime()
{
  //todo tick to only do this every X seconds;
  //todo keep track of rollover)
  if(timeloop<1000)
  {
    timeloop++;
    return;
  }
  timeloop=0;
  //rollover
  long now = millis();
  if((rollOver && now<lastDayStart && now>=nextDayStart) ||(!rollOver && now>=nextDayStart))
  {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("     >>NEW DAY<<");
    display.println("     >>NEW DAY<<");
    display.println("     >>NEW DAY<<");
    display.println("     >>NEW DAY<<");
    display.display();
    //we are in the new day
    //move all the values:
    for(byte d =0;d<6;d++)
    {
      last7Days[d+1]=last7Days[d];
    }
    last7Days[0]=ticks;
    ticks =0;
    lastDayStart = nextDayStart;
    nextDayStart = nextDayStart + dayLength;
    
    if(nextDayStart < lastDayStart)
    {
      rollOver=true;
    }
    else
    {
      rollOver= false;
    }
    if(enableSerial)
    {
      Serial.print("Last day:");
      Serial.println(last7Days[0]);
    }

    triggerDisplayOff();
  }
  
  
  
}
void triggerDisplayOff()
{
  displayOn=true;
  turnOffDisplayTimer=0;
}

void turnOffDisplay()
{
  if(!displayOn)
  {
    return;
  }
  if(turnOffDisplayTimer<turnOffDisplayTimeout)
  {
    turnOffDisplayTimer++;  
  }
  else 
  {
    displayOn=false;
    display.clearDisplay();
    display.display();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  measure();
  updateDisplay();
  turnOffDisplay();
  keepTime();
  delay(1);
  
}
