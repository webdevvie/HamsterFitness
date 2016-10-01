#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define hallPin 0
#define standardDev 50
#define largewheel 0


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

Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN, NEO_GRB + NEO_KHZ800);


const long lastDayStart =0;
const long nextDayStart =0;


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
int timeLoop=0;
//todo write stuff for day rotation
boolean rollOver=false;
int turnOffDisplayTimeout=9000;
boolean displayOn=true;


void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  color = strip.Color(0,0,0);
  red = strip.Color(8,0,0);
  orange = strip.Color(4,4,0);
  purple = strip.Color(4,0,4);
  teal = strip.Color(0,4,4);
  blue = strip.Color(0,0,8);
  green = strip.Color(0,8,0);

  colorsPerLight [0] = red;
  colorsPerLight [1] = orange;
  colorsPerLight [2] = orange;
  colorsPerLight [3] = purple;
  colorsPerLight [4] = purple;
  colorsPerLight [5] = blue;
  colorsPerLight [6] = teal;
  colorsPerLight [7] = green;
  
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
  
  distancePerLight = healthyHamsterDistance / 8 ;

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
    strip.setPixelColor(p,green); 
    strip.show();
    delay(100);
  }
  //now figure out the avarage;
  for(p=0;p<8;p++)
  {
    total+=analogRead(hallPin);
    strip.setPixelColor(p,0); 
    strip.show();
    delay(100);
  }
  normalState = total/16;
  
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
    strip.setPixelColor(0,0);
    strip.setPixelColor(1,0);
    strip.setPixelColor(2,0);
    strip.setPixelColor(3,0);
    strip.setPixelColor(4,0);
    strip.setPixelColor(5,0);
    strip.setPixelColor(6,0);
    strip.setPixelColor(7,0);

    strip.show();
    return;
  }
  
}
void updateDisplay()
{
    if(!needsDisplayUpdate)
    {
       return;
    }
    
    needsDisplayUpdate=false;
    long distanceTraveled = distanceinMMTraveled();
    byte lights = distanceTraveled /distancePerLight;
    if(lights > 7)
    {
      lights = 7;
    }
    color = colorsPerLight[lights];
    
    for(byte l =0;l<8;l++)
    {
      
      if(l <=lights)
      {
        strip.setPixelColor(l,color);
      }
      else
      {
        strip.setPixelColor(l,0);
      }
    }
    strip.show();
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
  if((rollOver && now<lastStartDay && now>=nextStartDay) ||(!rollOver && now>=nextStartDay)
  {
    //we are in the new day
    //move all the values:
    for(byte d =0;d<6;d++)
    {
      last7Days[d+1]=last7Days[d];
    }
    last7Days[0]=ticks;
    ticks =0;
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
    strip.setPixelColor(0,red);
    strip.setPixelColor(1,orange);
    strip.setPixelColor(2,orange);
    strip.setPixelColor(3,purple);
    strip.setPixelColor(4,purple);
    strip.setPixelColor(5,blue);
    strip.setPixelColor(6,teal);
    strip.setPixelColor(7,green);
    strip.show();
    
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
    for(byte l =0;l<8;l++)
    {
      strip.setPixelColor(l,0);
    }
    strip.show();
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
