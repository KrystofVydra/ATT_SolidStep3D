#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EspMQTTClient.h"
#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include <stdio.h>
#include <FastLED.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

EspMQTTClient client(
  "164947",
  "bruh1234",
  "192.168.241.27",  // MQTT Broker server ip
  "",                // Can be omitted if not needed
  "",                // Can be omitted if not needed
  "TestClient2",     // Client name that uniquely identify your device
  1883               // The MQTT port, default to 1883. this line can be omitted
);


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define NUM_LEDS 8
#define DATA_PIN 15
uint8_t displayScrIndex = 0;

#define TEMP_LOW 20
#define TEMP_HIGH 28

#define COUNT_LOW 1638
#define COUNT_HIGH 7864
#define TIMER_WIDTH 16

#define DEFAULT_HEATER_POSITION 90

#include "esp32-hal-ledc.h"

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

CRGB leds[NUM_LEDS];


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES 10  // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16

float v = 0;
uint8_t red, blue;
bool ledEnabled = true;


void setup() {
  Serial.begin(115200);
  delay(100);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages();  // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater();     // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  if (!sht31.begin(0x44)) {  // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  ledcSetup(2, 50, TIMER_WIDTH);  // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(19, 2);

  servoSetAngle(DEFAULT_HEATER_POSITION);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed




  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000);  // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();

  display.clearDisplay();

  testdrawchar();
  display.display();

  clearDisplay();

  for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
    leds[ledIndex] = CRGB(1, 1, 1);
  }

  FastLED.show();


  // display.setCursor(0, 0);
  // display.setTextSize(1);      // Normal 1:1 pixel scale
  // display.print("teplota: ");
  // int teplota = ;
  // display.print(teplota);
  // display.print(" ");
  // display.cp437(true);         // Use full 256 char 'Code Page 437' font
  // int16_t boo = 248;
  // display.write(boo);
  // display.print("C");
  // display.display();

  // Show the display buffer on the screen. You MUST call display() after
}


void onConnectionEstablished() {
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("home/devices/heat_max/heater", [](const String& heaterAngle) {
    Serial.println(heaterAngle);
    v = map(heaterAngle.toInt(), 0, 180, 0, 100);
    servoSetAngle(heaterAngle.toInt());
  });

  client.subscribe("home/devices/heat_max/led", [](const String& ledEnableData) {
    Serial.println(ledEnableData);
    //v = map(heaterAngle.toInt(), 0, 180, 0, 100);
    ledEnabled = ledEnableData.toInt();
  });


  // Publish a message to "mytopic/test"
  //client.publish("home/window_ctrl/", "This is a message");  // You can activate the retain flag by setting the third parameter to true
}

void loop() {

  client.loop();

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  if (!isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = ");
    Serial.print(t);
    Serial.print("\t\t");
    //displayShowTemp(t);
    ws2812Blink(0);
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", t);
    client.publish("home/devices/heat_max/temperature", msg);
    delay(10);
    ws2812Blink(1);
  } else {
    Serial.println("Failed to read temperature");
  }

  if (!isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = ");
    Serial.println(h);
    Serial.print("Temp *C = ");
    Serial.print(h);
    Serial.print("\t\t");
    //displayShowHum(h);
    ws2812Blink(0);
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", h);
    client.publish("home/devices/heat_max/humidity", msg);
    delay(10);
    ws2812Blink(1);

  } else {
    Serial.println("Failed to read humidity");
  }


  switch (displayScrIndex) {
  case 0:
    displayShowTemp(t);
    break;
  case 1:
    displayShowHum(h);
    break;
  case 2:
    displayShowValve(v);
    break;
  default:
    break;
}

    if (ledEnabled)
    {
      ws2812Update(t);
    }

    if (!ledEnabled)
    {
      if (true){
        for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
          leds[ledIndex] = CRGB::Black;
        }
        FastLED.show();
      }

    }
    changeIndex();
    delay(5000);
}




void clearDisplay() {
  display.clearDisplay();
  display.display();
}

void servoSetAngle(int param) {

  if (param >= 0 && param <= 180) {
    ledcWrite(2, map(param, 0, 180, COUNT_LOW, COUNT_HIGH));
    Serial.print("Servo angle written:");
    Serial.println(param);
  }
}



void displayShowTemp(float inputTemp) {

  display.clearDisplay();

  display.setCursor(4, 3);
  display.setTextSize(2);
  display.print("T");

  float dsc = inputTemp * 10;
  display.setCursor(20, 18);
  display.setTextSize(3);
  display.print((float)dsc / 10, 1);


  display.cp437(true);  // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);
  display.setCursor(100, 18);
  int16_t boo = 248;
  display.write(boo);
  display.print("C");
  display.display();
}

void displayShowHum(float inputHum) {

  display.clearDisplay();

  display.setCursor(4, 3);
  display.setTextSize(2);
  display.print("H");

  float dsc = inputHum * 10;
  display.setCursor(40, 18);
  display.setTextSize(3);
  display.print((float)dsc / 10, 0);


  display.cp437(true);  // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);
  display.setCursor(85, 25);
  display.print("%");
  display.display();
}

void displayShowValve(float inputValve) {

  display.clearDisplay();

  display.setCursor(4, 3);
  display.setTextSize(2);
  display.print("V");

  float dsc = inputValve * 10;
    if (inputValve < 100)
  {
      display.setCursor(40, 18);
  }
  
  else if (inputValve == 100)
  {
    display.setCursor(30, 18);
  }

  //display.setCursor(40, 18);
  display.setTextSize(3);
  display.print((float)dsc / 10, 0);


  display.cp437(true);  // Use full 256 char 'Code Page 437' font
  display.setTextSize(2);
  if (inputValve < 100)
  {
      display.setCursor(85, 25);
  }
  
  else if (inputValve == 100)
  {
    display.setCursor(105, 25);
  }

  display.print("%");
  display.display();
}

void changeIndex() {
  if (displayScrIndex < 2) {
    displayScrIndex++;
    return;
  }

  else if (displayScrIndex = 2) {
    displayScrIndex = 0;
    return;
  }
}


void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}


void ws2812Update(float temp)
{
red = map(temp, TEMP_LOW, TEMP_HIGH, 0, 255);
blue = map(temp, TEMP_LOW, TEMP_HIGH, 255, 0);

for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
  leds[ledIndex] = CRGB(red, blue, 0);
}

if (ledEnabled) {FastLED.show();}
if (!ledEnabled)
{
  FastLED.clear();
  FastLED.show();
}
}

void ws2812Blink(bool state)
{
  if (!state){
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
      leds[ledIndex] = CRGB::Black;
    }
  }

  if (state){
    for (int ledIndex = 0; ledIndex < NUM_LEDS; ledIndex++) {
      leds[ledIndex] = CRGB(red, blue, 0);
    }
  }

if (ledEnabled) {FastLED.show();}
if (!ledEnabled)
{
  FastLED.clear();
  FastLED.show();
}
}