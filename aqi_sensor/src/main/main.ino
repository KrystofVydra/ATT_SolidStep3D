#include <SoftwareSerial.h>
//#include <ArduinoJson.h>
#include <FS.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

struct particleSensorState_t {
  uint16_t avgPM25 = 0;
  uint16_t measurements[5] = { 0, 0, 0, 0, 0 };
  uint8_t measurementIdx = 0;
  boolean valid = false;
};

//constexpr static const uint8_t PIN_UART_RX = 4;   // D2 on Wemos D1 Mini
//constexpr static const uint8_t PIN_UART_TX = 13;  // UNUSED
#define PIN_UART_RX 4
#define PIN_UART_TX 13

particleSensorState_t jebat;



SoftwareSerial sensorSerial(PIN_UART_RX, PIN_UART_TX);

uint8_t serialRxBuf[255];
char buffer[25] = "";
uint8_t rxBufIdx = 0;

//wifi and mqtt setup
const char* ssid = "164947";
const char* password = "bruh1234";
const char* mqtt_server = "raspberrypi.local";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("home/sensors/aqi/PM2.5", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//end




void setup() {
  Serial.begin(9600);
  sensorSerial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("zacatekkodu");
}

void clearRxBuf() {
  // Clear everything for the next message
  memset(serialRxBuf, 0, sizeof(serialRxBuf));
  rxBufIdx = 0;
}

void parseState(particleSensorState_t& state) {
  /**
         *         MSB  DF 3     DF 4  LSB
         * uint16_t = xxxxxxxx xxxxxxxx
         */
  const uint16_t pm25 = (serialRxBuf[5] << 8) | serialRxBuf[6];

  Serial.printf("Received PM 2.5 reading: %d\n", pm25);

  state.measurements[state.measurementIdx] = pm25;

  state.measurementIdx = (state.measurementIdx + 1) % 5;

  if (state.measurementIdx == 0) {
    float avgPM25 = 0.0f;

    for (uint8_t i = 0; i < 5; ++i) {
      avgPM25 += state.measurements[i] / 5.0f;
    }

    state.avgPM25 = avgPM25;
    state.valid = true;

    Serial.printf("New Avg PM25: %d\n", state.avgPM25);
    snprintf(msg, MSG_BUFFER_SIZE, "%ld", state.avgPM25);
    client.publish("home/sensors/aqi/PM2.5", msg);
  }

  clearRxBuf();
}

bool isValidHeader() {
  bool headerValid = serialRxBuf[0] == 0x16 && serialRxBuf[1] == 0x11 && serialRxBuf[2] == 0x0B;

  if (!headerValid) {
    Serial.println("Received message with invalid header.");
  }

  return headerValid;
}

bool isValidChecksum() {
  uint8_t checksum = 0;

  for (uint8_t i = 0; i < 20; i++) {
    checksum += serialRxBuf[i];
  }

  if (checksum != 0) {
    Serial.printf("Received message with invalid checksum. Expected: 0. Actual: %d\n", checksum);
  }

  return checksum == 0;
}

void handleUart(particleSensorState_t& state) {
  if (!sensorSerial.available()) {
    return;
  }

  Serial.print("Receiving:");
  while (sensorSerial.available()) {
    serialRxBuf[rxBufIdx++] = sensorSerial.read();
    Serial.print(".");

    // Without this delay, receiving data breaks for reasons that are beyond me
    delay(15);

    if (rxBufIdx >= 64) {
      clearRxBuf();
    }
  }
  Serial.println("Done.");

  if (isValidHeader() && isValidChecksum()) {
    parseState(state);

    Serial.printf(
      "Current measurements: %d, %d, %d, %d, %d\n",

      state.measurements[0],
      state.measurements[1],
      state.measurements[2],
      state.measurements[3],
      state.measurements[4]);
  } else {
    clearRxBuf();
  }
}


void loop() {
  handleUart(jebat);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
