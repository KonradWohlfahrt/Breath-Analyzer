#include <U8x8lib.h>
#include <DHT.h>
#include <EEPROM.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define BATPIN A0
#define SENSOR A1

#define LEFTBUTTON 4
#define RIGHTBUTTON 7

#define BUZZER 3
#define BUZZERVOLUME 127
#define BUZZERTIME 15

#define LOWBATLED 5
#define SLEEPLED 6
#define GREENLED 9
#define REDLED 10


// enter your output voltage here or leave at 5V
#define VOLTS 5.0f
// leave if VOLTS=5.0f, or calculate by: (3.9V, 3.7V, 3.6V, 3.55V) / VOLTS * 1023s
#define FULLCHARGE 798
#define NORMALCHARGE 757
#define LOWCHARGE 737
#define EMPTYCHARGE 726

// enter your load resistor here if changed
#define R6 4700
// clean air factor from datasheet: rs/r0 in clean air is 60
#define CLEANAIRFACTOR 60.0f


// gas concentration graph, values from datasheet
// P1(50ppm, 0.17 Rs/R0) 
// P2(500ppm, 0.0225 Rs/R0)
const float point1[] = { log10(50), log10(0.17) };
const float point2[] = { log10(500), log10(0.0225) };
const float scope = (point2[1] - point1[1]) / (point2[0] - point1[0]);
const float coord = point1[1] - point1[0] * scope;

// display tiles
const uint8_t batteryBackground[24] = { 0x00, 0x00, 0x7e, 0x81, 0x81, 0x81, 0x81, 0x81,
  0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
  0x81, 0x81, 0x81, 0xc3, 0x7e, 0x18, 0x00, 0x00 };
const uint8_t oneBatteryBar[8] = { 0x00, 0x00, 0x7e, 0x81, 0xbd, 0xbd, 0xbd, 0xbd };
const uint8_t twoBatteryBars[8] = { 0x81, 0xbd, 0xbd, 0xbd, 0xbd, 0x81, 0x81, 0x81 };
const uint8_t threeBatteryBars[16] = { 0x81, 0xbd, 0xbd, 0xbd, 0xbd, 0x81, 0xbd, 0xbd, 0xbd, 0xbd, 0x81, 0xc3, 0x7e, 0x18, 0x00, 0x00 };
const uint8_t arrowUpperTile[8] = { 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00, 0x00 };
const uint8_t arrowLowerTile[8] = { 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0x00 };

// text
const char* loadingTxt = "Loading...";
const char* soundTxt = "Sound";
const char* contrastTxt = "Contrast";
const char* calibrateTxt = "Calibrate";
const char* calculateTxt = "Calculating...";
const char* heatingTxt = "Heating...";
const char* blowTxt = "Blow...";
const char* bacTxt = "BAC: ";
const char* onTxt = "on";
const char* offTxt = "off";


U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(U8X8_PIN_NONE);
DHT dht(DHTPIN, DHTTYPE);

bool sleeping = false;
unsigned long interactionTimestamp;
unsigned long tempCheckTimestamp;
unsigned long batteryCheckTimestamp;

unsigned int R0;

uint8_t settingsIndex = 0;
uint8_t contrast;
bool buzzerActive;


bool modeButton() { return digitalRead(LEFTBUTTON) == 0; }
bool setButton() { return digitalRead(RIGHTBUTTON) == 0; }
bool anyButton() { return modeButton() || setButton(); }
bool allButtons() { return modeButton() && setButton(); }

float analogToVolt(int value) { return (float)value / 1023.0f * VOLTS; }
float getSensorResistance(int adc) { return ((float)R6 * (1023.0f - adc) / adc); }
float getConcentration(float resistance) { return pow(10, coord + scope * log(resistance / R0)); }

bool isTimestamp(unsigned long& timestamp, int interval) 
{
  if (millis() - timestamp >= interval)
  {
    timestamp = millis();
    return true;
  }
  return false;
}
void setBuzzerIgnore(int time)
{
  analogWrite(BUZZER, BUZZERVOLUME);
  delay(time);
  analogWrite(BUZZER, 0);
}
void setBuzzer(int time)
{
  if (!buzzerActive)
    return;
  setBuzzerIgnore(time);
}
void setBuzzer() { setBuzzer(BUZZERTIME, false); }


void setup() 
{
  // initialize pins
  pinMode(BATPIN, INPUT);
  pinMode(SENSOR, INPUT);
  pinMode(LEFTBUTTON, INPUT);
  pinMode(RIGHTBUTTON, INPUT);

  pinMode(BUZZER, OUTPUT);
  pinMode(LOWBATLED, OUTPUT);
  pinMode(SLEEPLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  pinMode(REDLED, OUTPUT);

  // initialize display, dht11 sensor
  u8x8.begin();
  u8x8.setFont(u8x8_font_pressstart2p_f);
  u8x8.draw1x2String(0, 2, loadingTxt);
  dht.begin();
  
  // read settings from eeprom
  EEPROM.begin(); // 2 bytes (R0), 1 byte (contrast), 1 byte (sound)
  delay(250);
  EEPROM.get(0, R0);
  contrast = EEPROM.read(2);
  buzzerActive = EEPROM.read(3) == 1;

  u8x8.setContrast(contrast);

  setBuzzer();
  heating(30000); // heat for 30 seconds to get stable readings

  showStartScreen();
  interactionTimestamp = millis();
}
void loop() 
{
  if (sleeping)
  {
    checkBattery(false);
    if (anyButton())
    {
      analogWrite(SLEEPLED, 0);
      while (anyButton());
      sleeping = false;
      interactionTimestamp = millis();
      showStartScreen();
      u8x8.setPowerSave(0);
    }
    return;
  }
  // sleep after 5s
  else if (isTimestamp(interactionTimestamp, 5000)) 
  {
    sleeping = true;
    u8x8.setPowerSave(1);
    analogWrite(SLEEPLED, 127);
    return;
  }

  // handle input
  if (anyButton())
  {
    bool both = false;
    while (anyButton())
    {
      checkBattery(false);
      displayTemp(false);
      if (!both && allButtons())
        both = true;
    }

    if (!both)
      measure();
    else
      settings();

    showStartScreen();
    interactionTimestamp = millis();
  }

  checkBattery(false);
  displayTemp(false);
}

void settings()
{
  setBuzzer();
  settingsIndex = 0;
  u8x8.clearDisplay();
  checkBattery(true);
  showSettings();

  while (true)
  {
    checkBattery(false);
    if (anyButton())
    {
      bool set = setButton();
      bool both = false;
      while (anyButton())
      {
        checkBattery(false);
        if (!both && allButtons())
          both = true;
      }
      
      if (both)
        break;
      else
      {
        if (set)
        {
          if (settingsIndex == 0)
          {
            buzzerActive = !buzzerActive;
            setBuzzer();
          }
          else if (settingsIndex == 1)
          {
            if (contrast == 0)
              contrast = 50;
            else if (contrast == 50)
              contrast = 127;
            else if (contrast == 127)
              contrast = 255;
            else
              contrast = 0;
            u8x8.setContrast(contrast);
            setBuzzer();
          }
          else
            calibrate();
        }
        else
        {
          settingsIndex++;
          if (settingsIndex > 2)
            settingsIndex = 0;
          setBuzzer();
        }

        showSettings();
      }
    }
  }

  setBuzzer();

  EEPROM.update(2, contrast);
  EEPROM.update(3, buzzerActive);
}
void calibrate()
{
  setBuzzer();
  heating(600000); // heat for 10 minutes

  setBuzzer(BUZZERTIME * 2);
  delay(BUZZERTIME * 2);
  setBuzzer(BUZZERTIME * 2);

  u8x8.clearDisplay();
  u8x8.draw1x2String(0, 2, calculateTxt);

  // get average of 50 analog reads
  unsigned int avg = 0;
  for (uint8_t i = 0; i < 50; i++)
  {
    avg += analogRead(SENSOR);
    delay(200);
  }
  avg /= 50;

  R0 = (unsigned int)(getSensorResistance(avg) / CLEANAIRFACTOR); // divide by clean air factor (datasheet) to get r0

  u8x8.clearDisplay();
  u8x8.drawString(0, 1, (String(avg) + " - " + String(analogToVolt(avg), 2) + "V").c_str());
  u8x8.drawString(0, 2, ("R0: " + String(R0)).c_str());

  EEPROM.put(0, R0);

  delay(5000);
}
void measure()
{
  // heat
  setBuzzer();
  heating(5000);
  
  // blow
  setBuzzer();
  int reading = getBlowSamples();

  // calculate bac
  // ppm * 0.9988590004 = mg/l
  // mg/l * 2 = per mille
  float bac = getConcentration(getSensorResistance(reading)) * 1.997718f;

  // display bac
  u8x8.clearDisplay();
  u8x8.draw1x2String(0, 2, (bacTxt + String(bac, 3)).c_str());
  checkBattery(true);
  
  if (bac >= 0.5f)
  {
    analogWrite(REDLED, 255);
    setBuzzer(BUZZERTIME * 2);
  }
  else 
  {
    analogWrite(GREENLED, 255);
    setBuzzer();
  }
    
  delay(5000);
  analogWrite(REDLED, 0);
  analogWrite(GREENLED, 0);
}


void heating(unsigned long time)
{
  u8x8.clearDisplay();
  u8x8.draw1x2String(0, 2, heatingTxt);
  checkBattery(true);

  uint8_t v = 0;
  bool up = true;

  unsigned long start = millis();
  int lastSec = 0;
  while (millis() - start < time)
  {
    unsigned long remaining = time - (millis() - start);
    int seconds = remaining / 1000 % 60;

    if (lastSec != seconds)
    {
      lastSec = seconds;
      int minutes = remaining / 1000 / 60;
      u8x8.drawString(0, 0, ((minutes < 10 ? "0" + String(minutes) : String(minutes)) + ":" + (seconds < 10 ? "0" + String(seconds) : String(seconds))).c_str());
    }

    checkBattery(false);

    if (up)
      v++;
    else
      v--;
    if (v == 255 || v == 0)
      up = !up;
    
    analogWrite(REDLED, v);
    delay(2);
  }
  analogWrite(REDLED, 0);
}
int getBlowSamples()
{
  u8x8.clearDisplay();
  u8x8.draw1x2String(0, 2, blowTxt);
  checkBattery(true);

  analogWrite(GREENLED, 255);

  unsigned int avg = 0;
  int amount = 0;
  unsigned long start = millis();
  while (millis() - start < 4500)
  {
    unsigned long remaining = 4500 - (millis() - start);
    u8x8.draw1x2String(13, 2, ("0" + String(remaining / 1000)).c_str());

    if (remaining <= 1500)
    {
      avg += analogRead(SENSOR);
      amount++;
      
      delay(20);
    }

    checkBattery(false);
  }
  analogWrite(GREENLED, 0);
  avg /= amount;
  return avg;
}


void showStartScreen()
{
  u8x8.clearDisplay();
  checkBattery(true);
  displayTemp(true);
}
void showSettings()
{
  u8x8.clearLine(1);
  u8x8.clearLine(2);
  u8x8.clearLine(3);

  u8x8.drawTile(0, 2, 1, arrowUpperTile);
  u8x8.drawTile(0, 3, 1, arrowLowerTile);
  switch (settingsIndex)
  {
    case 0:
      u8x8.draw1x2String(1, 2, soundTxt);
      if (buzzerActive)
        u8x8.draw1x2String(13, 2, onTxt);
      else
        u8x8.draw1x2String(12, 2, offTxt);
      break;
    case 1:
      u8x8.draw1x2String(1, 2, contrastTxt);
      if (contrast < 100)
        u8x8.draw1x2String(13, 2, String(contrast).c_str());
      else
        u8x8.draw1x2String(13, 2, String(contrast).c_str());
      break;
    case 2:
      u8x8.draw1x2String(1, 2, calibrateTxt);
      u8x8.drawString(0, 1, ("R0: " + String(R0)).c_str());
      break;
  }
}


void checkBattery(bool force)
{
  if (!force && !isTimestamp(batteryCheckTimestamp, 2000))
    return;

  unsigned int avg = 0;
  for (uint8_t b = 0; b < 15; b++)
  {
    avg += analogRead(BATPIN);
    delay(5);
  }
  avg = avg / 15;

  if (avg >= FULLCHARGE)
  {
    displayBattery(3);
  }
  else if (avg >= NORMALCHARGE)
  {
    displayBattery(2);
  }
  else if (avg >= LOWCHARGE)
  {
    displayBattery(1);
  }
  else if (avg >= EMPTYCHARGE)
  {
    displayBattery(0);
    analogWrite(LOWBATLED, 128);
  }
  else
  {
    setBuzzerIgnore(BUZZERTIME * 2);delay(BUZZERTIME * 2);setBuzzerIgnore(BUZZERTIME * 2);
    analogWrite(SLEEPLED, 0);
    analogWrite(GREENLED, 0);
    analogWrite(REDLED, 0);
    analogWrite(BUZZER, 0);
    u8x8.setPowerSave(1);
    while (true) 
    {
      analogWrite(LOWBATLED, 32);
      delay(500);
      analogWrite(LOWBATLED, 0);
      delay(1500);
    }
  }

  if (!sleeping)
    u8x8.drawString(7, 0, (String(analogToVolt(avg), 2) + "V").c_str());
}
void displayBattery(uint8_t i)
{
  if (sleeping)
    return;
  u8x8.drawTile(13, 0, 3, batteryBackground);
  if (i != 0)
    u8x8.drawTile(13, 0, 1, oneBatteryBar);
  if (i == 2)
    u8x8.drawTile(14, 0, 1, twoBatteryBars);
  else if (i == 3)
    u8x8.drawTile(14, 0, 2, threeBatteryBars);
}
void displayTemp(bool force)
{
  if (sleeping)
    return;
  if (!force && !isTimestamp(tempCheckTimestamp, 2000))
    return;

  String t = String(dht.readTemperature(), 1);
  String h = String((int)dht.readHumidity());

  char txt[12] = { t[0], t[1], '.', t[3], '°', 'C', ' ', ' ', h[0], h[1], '%', '\0' };
  if (h.length() >= 3)
  {
    txt[7] = h[0];
    txt[8] = h[1];
    txt[9] = h[2];
  }
  
  u8x8.draw1x2String(3, 2, txt);
}