// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#define ONE_WIRE_BUS 4

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_GREEN 10
#define LED_YELLOW 9
#define BTN_READY 3
#define BTN_START 4

#define stable_time 7500UL

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);

DeviceAddress insideThermometer;

float value = 0;

int pwr = 0;
int pn = 1;
int led_step = 16;

void setup(void)
{
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(BTN_READY, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  sensors.setResolution(insideThermometer, 10);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextSize(1);             
  display.setTextColor(SSD1306_WHITE);        
  display.setCursor(28,12);             
  display.println(F("Push"));
  display.setCursor(4,30);             
  display.println(F("The button"));
  display.display();
  //display.setFont(&FreeMonoBold18pt7b);
}

void(* resetFunc) (void) = 0;

// function to print the temperature for a device
float printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  return tempC;
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void start() {
  while (digitalRead(BTN_READY))
  {
    analogWrite(LED_YELLOW, pwr);
    analogWrite(LED_GREEN, 255 - pwr);

    pwr += led_step * pn;
    if (pn > 0 && pwr > 255) {
      pwr -= led_step;
      pn = -1;
    }
    else if (pn < 0 && pwr < 0) {
      pwr += led_step;
      pn = 1;
    }

    delay(35);
  }

  analogWrite(LED_YELLOW, 255);
  analogWrite(LED_GREEN, LOW);
}

int stable() {
  unsigned long last_time = millis();
  unsigned long led_time = millis();
  const unsigned long led_limit = 500UL;
  float last_value;

  digitalWrite(LED_YELLOW, HIGH);

  while (1) {
    sensors.requestTemperatures();
    value = printTemperature(insideThermometer);

    display.clearDisplay();
    display.setCursor(10,28); 
    display.println(value);  
    display.display();

    Serial.println(value);
    if (value != last_value) {
      last_value = value;
      last_time = millis();
    }

    if (millis() - last_time >= stable_time) break;
    if (millis() - led_time >= led_limit) {
      digitalWrite(LED_YELLOW, !digitalRead(LED_YELLOW));
      led_time = millis();
    }
  }

  display.clearDisplay();
  display.setCursor(0,12);             
  display.println(F("Stable val:"));
  display.setCursor(4,30);             
  display.println(value);
  display.display();

  digitalWrite(LED_YELLOW, HIGH);
  while (digitalRead(BTN_READY)) {
    analogWrite(LED_GREEN, 255 - pwr);

    pwr += led_step * pn;
    if (pn > 0 && pwr > 255) {
      pwr -= led_step;
      pn = -1;
    }
    else if (pn < 0 && pwr < 0) {
      pwr += led_step;
      pn = 1;
    }

    delay(35);
  }

  analogWrite(LED_YELLOW, 255);

  return value;
}

int measure() {
  unsigned long last_time = millis();
  unsigned long led_time = millis();
  const unsigned long led_limit = 500UL;
  float last_value;

  digitalWrite(LED_GREEN, HIGH);

  while (1) {
    sensors.requestTemperatures();
    value = printTemperature(insideThermometer);

    display.clearDisplay();
    display.setCursor(10,28); 
    display.println(value);  
    display.display();

    Serial.println(value);
    if (value > last_value) {
      last_value = value;
    }

    if (value < last_value) break;

    if (millis() - led_time >= led_limit) {
      digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
      led_time = millis();
    }

    delay(10);
  }

  display.clearDisplay();
  display.setCursor(0,12);             
  display.println(F("Final val:"));
  display.setCursor(4,30);             
  display.println(last_value);
  display.display();

  digitalWrite(LED_GREEN, HIGH);
  while (digitalRead(BTN_READY)) {
    analogWrite(LED_YELLOW, 255 - pwr);
    analogWrite(LED_GREEN, 255 - pwr);
    pwr += led_step * pn;
    if (pn > 0 && pwr > 255) {
      pwr -= led_step;
      pn = -1;
    }
    else if (pn < 0 && pwr < 0) {
      pwr += led_step;
      pn = 1;
    }

    delay(35);
  }

  return last_value;
}

void final(int val_st, int val_fin) {
  Serial.print("Stable value: ");
  Serial.println((float) val_st);
  Serial.print("Final value: ");
  Serial.println((float) val_fin);

  while (digitalRead(BTN_READY)) {

  }

  delay(1000);
  resetFunc();
}
/*
 * Main function. It will request the tempC from the sensors and display on Serial.
 */
void loop(void)
{ 
  float v_stable;
  float v_final;

  start();

  v_stable = stable();

  unsigned long op_time = millis();
  v_final = measure();  
  op_time = (millis() - op_time);

  Serial.print("Time: ");
  Serial.println(op_time / 1000);

  final(v_stable, v_final);
}


