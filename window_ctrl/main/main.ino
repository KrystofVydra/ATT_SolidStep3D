/*
  SimpleMQTTClient.ino
  The purpose of this exemple is to illustrate a simple handling of MQTT and Wifi connection.
  Once it connects successfully to a Wifi network and a MQTT broker, it subscribe to a topic and send a message to it.
  It will also send a message delayed 5 seconds later.
*/

#include "EspMQTTClient.h"


#define COUNT_LOW 1638
#define COUNT_HIGH 7864
#define TIMER_WIDTH 16

#define DEFAULT_BLINDS_POSITION 90
#define DEFAULT_WINDOW_POSITION 90

#include "esp32-hal-ledc.h"



EspMQTTClient client(
  "164947",
  "bruh1234",
  "192.168.241.27",  // MQTT Broker server ip
  "",                // Can be omitted if not needed
  "",                // Can be omitted if not needed
  "TestClientttt",   // Client name that uniquely identify your device
  1883               // The MQTT port, default to 1883. this line can be omitted
);

void setup() {
  Serial.begin(115200);

  // Optional functionalities of EspMQTTClient
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                         // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  //client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

  ledcSetup(1, 50, TIMER_WIDTH);  // channel 1, 50 Hz, 16-bit width
  ledcAttachPin(22, 1);           // GPIO 22 assigned to channel 1

  ledcSetup(2, 50, TIMER_WIDTH);  // channel 2, 50 Hz, 16-bit width
  ledcAttachPin(19, 2);           // GPIO 19 assigned to channel 2


  //set default servo angle
  servoSetAngle(DEFAULT_BLINDS_POSITION, 1);
  servoSetAngle(DEFAULT_WINDOW_POSITION, 2);
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("home/window_ctrl/blinds", [](const String& blindsAngle) {
    Serial.println(blindsAngle);
    servoSetAngle(blindsAngle.toInt(), 1);
  });

  client.subscribe("home/window_ctrl/window", [](const String& windowAngle) {
    Serial.println(windowAngle);
    servoSetAngle(windowAngle.toInt(), 2);
  });

  // Publish a message to "mytopic/test"
  //client.publish("home/window_ctrl/", "This is a message");  // You can activate the retain flag by setting the third parameter to true
}

void loop() {
  client.loop();
}



void servoSetAngle(int param, uint8_t servoSelect) {

  if (param >= 0 && param <= 180) {
    ledcWrite(servoSelect, map(param, 0, 180, COUNT_LOW, COUNT_HIGH));
    Serial.print("Servo angle written:");
    Serial.println(param);
  }
}
