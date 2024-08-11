#include <U8x8lib.h>
#include <DHT.h>
#include <EEPROM.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define SENSOR A1

#define LEFTBUTTON 4
#define RIGHTBUTTON 7


// enter your output voltage here or leave at 5V
#define VOLTS 5.0f

// enter your resistor here if changed
#define R6 4700
// R0 = sensor resistance (in clean air) / 60.0f
#define R0 267

// gas concentration graph
const float point1[] = { log10(50), log10(0.17) };
const float point2[] = { log10(500), log10(0.0225) };
const float scope = (point2[1] - point1[1]) / (point2[0] - point1[0]);
const float coord = point1[1] - point1[0] * scope;


U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(U8X8_PIN_NONE);
DHT dht(DHTPIN, DHTTYPE);


int index = 0;
unsigned long timestp;


bool modeButton() { return digitalRead(LEFTBUTTON) == 0; }
bool setButton() { return digitalRead(RIGHTBUTTON) == 0; }
bool anyButton() { return modeButton() || setButton(); }

float getSensorResistance(int adc) { return ( ((float)R6 * (1023.0f - adc) / adc)); }
float getConcentration(float resistance) { return pow(10, coord + scope * log(resistance / R0)); }
float analogToVolt(int value) { return (float)value / 1023.0f * VOLTS; }

bool isTimestamp(unsigned long& timestamp, int interval) 
{
  if (millis() - timestamp >= interval)
  {
    timestamp = millis();
    return true;
  }
  return false;
}


void setup() 
{
  // initialize pins
  pinMode(SENSOR, INPUT);
  pinMode(LEFTBUTTON, INPUT);
  pinMode(RIGHTBUTTON, INPUT);

  // initialize display, dht11 sensor
  u8x8.begin();
  u8x8.setFont(u8x8_font_pressstart2p_f);
  dht.begin();
}
void loop() 
{
  if (anyButton()) {
    while (anyButton());
    index = (index + 1) % 5;
    updateDisplay();
    timestp = millis();
  }

  if (isTimestamp(timestp, 500))
    updateDisplay();
}

void updateDisplay()
{
  u8x8.clearLine(2);
  u8x8.clearLine(3);

  unsigned int avg = 0;
  for (uint8_t i = 0; i < 50; i++)
  {
    avg += analogRead(SENSOR);
    delay(2);
  }
  avg /= 50;

  if (index == 0)
  {
    u8x8.draw1x2String(0, 2, String(avg).c_str());
  }
  else if (index == 1)
  {
    float sensorVolt = analogToVolt(avg);
    u8x8.draw1x2String(0, 2, (String(sensorVolt, 2) + "V").c_str());
  }
  else if (index == 2)
  {
    unsigned int r = getSensorResistance(avg);
    u8x8.draw1x2String(0, 2, String(r).c_str());
  }
  else if (index == 3)
  {
    float bac = getConcentration(getSensorResistance(avg)) * 1.997718f; // ppm to per mille
    u8x8.draw1x2String(0, 2, String(bac, 3).c_str());
  }
  else
  {
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
}