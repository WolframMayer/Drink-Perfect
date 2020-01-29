#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Losant.h>

#include <OneWire.h> 
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);p
DallasTemperature sensors(&oneWire);

// WiFi credentials.
const char* WIFI_SSID = "w";
const char* WIFI_PASS = "123456789";

// Losant credentials.
const char* LOSANT_DEVICE_ID = "5dd93b900ac5cc0007fbfdb7";
const char* LOSANT_ACCESS_KEY = "5284cf5a-77c5-4f11-8041-27712f938018";
const char* LOSANT_ACCESS_SECRET = "a4c494f9caa728f293986a73b6bb4855c65b69a9464db72e1de21d8cdee89424";

//WiFiClientSecure wifiClient;
WiFiClient wifiClient;
LosantDevice device(LOSANT_DEVICE_ID);
void connect() {
  // Connect to Wifi.

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  //WiFi.persistent(false);
  //WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {

      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if(millis() - wifiConnectStart > 5000) {

      Serial.println("Failed to connect to WiFi");   
      return;
    }
  }


  Serial.println("WiFi connected");

  HTTPClient http;
  http.begin("http://api.losant.com/auth/device");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");

  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = LOSANT_DEVICE_ID;
  root["key"] = LOSANT_ACCESS_KEY;
  root["secret"] = LOSANT_ACCESS_SECRET;
  String buffer;
  root.printTo(buffer);
  int httpCode = http.POST(buffer);

  if(httpCode > 0) {

      if(httpCode == HTTP_CODE_OK) {
          Serial.println("This device is authorized!");
      } else {
        Serial.println("Failed to authorize device to Losant.");
        if(httpCode == 400) {
          Serial.println("Validation error: The device ID, access key, or access secret is not in the proper format.");
        } else if(httpCode == 401) {
          Serial.println("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.");
        } else {
           Serial.println("Unknown response from API");
        }
      }
    } else {
        Serial.println("Failed to connect to Losant API.");
   }
  http.end();

  // Connect to Losant.

  Serial.print("Connecting to Losant...");
device.connect(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);
  //device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {

    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  // Wait for serial to initialize.
  while(!Serial) { }
  Serial.println("Device Started");
  connect();
}



void report(int tempC) {

  StaticJsonBuffer<500> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["tempC"] = tempC;
  Serial.println(tempC);
  device.sendState(root);
  Serial.println("Reported!");
}



int timeSinceLastRead = 0;
void loop() {
   bool toReconnect = false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }
  device.loop();

  // Report every 2 seconds.

  if(timeSinceLastRead > 2000) {

    // Read temperature as Celsius (the default)
    sensors.requestTemperatures();
    int t = sensors.getTempCByIndex(0);

    // Read temperature as Fahrenheit (isFahrenheit = true)

    // Check if any reads failed and exit early (to try again).

    if (isnan(t)) {

      Serial.println("Failed to read from sensor!");
      timeSinceLastRead = 0;
      return;
    }

    report(t);
    timeSinceLastRead = 0;
  }

  delay(100);
  timeSinceLastRead += 100;

}
