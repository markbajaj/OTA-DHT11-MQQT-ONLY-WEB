#include <DHT.h>

#include <ESP8266WebServer.h>

// Get ESP8266 going with Arduino IDE
// - https://github.com/esp8266/Arduino#installing-with-boards-manager
// Required libraries (sketch -> include library -> manage libraries)
// - PubSubClient by Nick ‘O Leary
// - DHT sensor library by Adafruit
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>  
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "Wire.h"


#define mqtt_server "192.168.101.25"
#define mqtt_user ""
#define mqtt_password ""

#define humidity_topic "sensor3/humidity3"
#define temperature_topic "sensor3/temperature3"
#define DHTPIN 4     // Connected to Pin D1 in NodeMCU
#define DHTTYPE DHT11


// ***************************************************************************
// Load library "ticker" for blinking status led
// ***************************************************************************
#include <Ticker.h>
Ticker ticker;
ESP8266WebServer server(80);
float humidity, temp_c;  // Values read from sensor
String webString="";     // String to display
String temp1="";
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor
 
void handle_root() {
  server.send(200, "text/html", "Hello from the weather esp8266, read from /temp or /humidity of /env");
  delay(100);
}

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

WiFiClient espClient12;
PubSubClient client(espClient12);
DHT dht(DHTPIN, DHTTYPE, 15); // 11 works fine for ESP8266


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
 // entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_c = dht.readTemperature(false);     // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_c)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  // set builtin led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
 //  start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);


  // ***************************************************************************
  // Setup: WiFiManager
  // ***************************************************************************
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect();


ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  

// OTA Bit *****************************************
//  Serial.begin(115200);
//  Serial.println("Booting");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("ESP8266-DHT11-2");

  // No authentication by default uncomment below and change password
  //ArduinoOTA.setPassword((const char *)"password");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

// OTA End **************************************************
  
  client.setServer(mqtt_server, 1883);

 server.on("/", handle_root);
  
  server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString="<h1> Temperature: </h1>"+String((int)temp_c)+" C";   // Arduino has a hard time with float to string
    server.send(200, "text/html", webString);            // send to someones browser when asked
  });
 
  server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();           // read sensor
    webString="<h1>Humidity: </h1>"+String((int)humidity)+"%";
    server.send(200, "text/html", webString);               // send to someones browser when asked
  });

  server.on("/env", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString= "<h1><center><u>Mark's ESP sensor3/# </u></center></h1><hr><h1>Temperature: </h1>"+String((int)temp_c)+"&#8451;"+"<h1> Humidity: </h1>"+String((int)humidity)+"%";;   // Arduino has a hard time with float to string
    server.send(200, "text/html", webString);            // send to someones browser when asked
  });
server.on("/test", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    temp1= "<html>\
  <head>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <form action='http://192.168.4.1' method='get'>\
    F_name: <input type='text' name='fname'><br>\
    <input type='submit' value='Submit'>\
</form>\
  </body>\
</html>",
//      hr, min % 60, sec % 60

  server.send(200, "text/html", temp1);
    });
  server.begin();
  Serial.println("HTTP server started");
}
 

 

void reconnect() {
  float testh = dht.readHumidity();
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Cl", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      Serial.println(temperature_topic);
      Serial.println(dht.readHumidity());
      Serial.println(dht.readTemperature());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
        (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

long lastMsg = 0;
float temp = 0;
float hum = 0;
float diff = 1.0;

void loop() {

    server.handleClient();

// OTA Bit ************************
  ArduinoOTA.handle();
// End OTA ************************ 
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = dht.readTemperature();
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
  }
}


