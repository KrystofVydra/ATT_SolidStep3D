#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "EspMQTTClient.h"
#include <Arduino.h>
#include "Adafruit_SHT31.h"
#include <stdio.h>

Adafruit_SHT31 sht31 = Adafruit_SHT31();

EspMQTTClient client(
  "164947",
  "bruh1234",
  "192.168.241.27",  // MQTT Broker server ip
  "",                // Can be omitted if not needed
  "",                // Can be omitted if not needed
  "TestClient2",   // Client name that uniquely identify your device
  1883               // The MQTT port, default to 1883. this line can be omitted
);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define COUNT_LOW 1638
#define COUNT_HIGH 7864
#define TIMER_WIDTH 16

#define DEFAULT_HEATER_POSITION 90

#include "esp32-hal-ledc.h"

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16


void setup() {
  Serial.begin(115200);
  delay(100);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }

  ledcSetup(2, 50, TIMER_WIDTH);  // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(19, 2);     

  servoSetAngle(DEFAULT_HEATER_POSITION);



  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  display.drawPixel(10, 10, SSD1306_WHITE);
  display.display();

  display.clearDisplay();
  
  testdrawchar();
  display.display();

  clearDisplay();


  display.setCursor(0, 0);
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.print("teplota: ");
  int teplota = 25;
  display.print(teplota);
  display.print(" ");
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  int16_t boo = 248;
  display.write(boo);
  display.print("C");
  display.display();

  // Show the display buffer on the screen. You MUST call display() after
 
}


void onConnectionEstablished() {
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("home/devices/heat_max/heater", [](const String& heaterAngle) {
    Serial.println(heaterAngle);
    servoSetAngle(heaterAngle.toInt());
  });



  // Publish a message to "mytopic/test"
  //client.publish("home/window_ctrl/", "This is a message");  // You can activate the retain flag by setting the third parameter to true
}

void loop() {

  client.loop();

  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
    if (! isnan(t)) {  // check if 'is not a number'
    Serial.print("Temp *C = "); Serial.print(t); Serial.print("\t\t");
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", t);
    client.publish("home/devices/heat_max/temperature", msg);
  } else { 
    Serial.println("Failed to read temperature");
  }
  
  if (! isnan(h)) {  // check if 'is not a number'
    Serial.print("Hum. % = "); Serial.println(h);
    Serial.print("Temp *C = "); Serial.print(h); Serial.print("\t\t");
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", h);
    client.publish("home/devices/heat_max/humidity", msg);

  } else { 
    Serial.println("Failed to read humidity");
  }

  delay(1000);


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



void clearDisplay()
{
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


