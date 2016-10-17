#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define hallPin 0
#define standardDev 50
#define largewheel 0
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
#define HAMSTERFITNESS_VERSION "0.1"
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);


#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif



/**
 * (c) 2016 John Bakker <me@johnbakker.name>
 * 
 * Hamster fitness measurement device
 */


const int largeWheelRadiusInMM= 85;
const int smallWheelRadiusInMM= 55;
//const long healthyHamsterDistance= 9000000; // 9km in mm (What a healthy hamster runs every night)
const long healthyHamsterDistance= 9000; // 900cm in mm for testing
//const long dayLength  = 86400000 //reset actual day length;
const long dayLength  = 60000;//reset every 60 seconds

const boolean enableSerial=true;

//Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);


 long lastDayStart =0;
long nextDayStart =0;


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
long distancePerLight=0;
boolean needsDisplayUpdate=false;
int timeloop=0;
int turnOffDisplayTimer=0;
//todo write stuff for day rotation
boolean rollOver=false;
int turnOffDisplayTimeout=9000;
boolean displayOn=true;


void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Hamster fitness v.");
  display.println(HAMSTERFITNESS_VERSION);
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
  
  distancePerLight = healthyHamsterDistance / 100 ;

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
  Serial.println("Normal state");
  Serial.println(normalState);
  
}

void updateSerial()
{

  if(enableSerial)
  {
    
  
    Serial.print("Distance traveled in MM");
       
    Serial.println(distanceinMMTraveled());
    Serial.print("Eight:");
    Serial.println(distancePerLight);
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
    //strip.setPixelColor(0,0);
    //strip.setPixelColor(1,0);
//    strip.setPixelColor(2,0);
//    strip.setPixelColor(3,0);
//    strip.setPixelColor(4,0);
//    strip.setPixelColor(5,0);
//    strip.setPixelColor(6,0);
//    strip.setPixelColor(7,0);

//    strip.show();
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
    long width = distanceTraveled /distancePerLight;
    
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
    for(byte l =0;l<8;l++)
    {
//      strip.setPixelColor(l,0);
    }
//    strip.show();
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
