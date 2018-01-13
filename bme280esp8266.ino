#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
//for LED status
#include <Ticker.h>

#include <Wire.h>
#include "SparkFunBME280.h"


//static const uint8_t D0 = 16;
static const int TICK_LED = 12; //D6
static const int MOISTURESENSOR_VCC = 13; //D7
//static const HUMIDITYSENSOR_PIN = ;
static const int LIGHTSENSOR = 15; //D8

float moisture = 0.0;
int light=0;

BME280 sensor; // I2C

Ticker ticker;


//IOT server
const char* server = "api.thingspeak.com";
String apiKey ="YW8HUEQSLQ3A6KHN";
#define myPeriodic 15 //in sec | Thingspeak pub is 15sec 
int sent = 0;

//temp, pressure, humidity
float h, t, p, pin, dp;
char temperatureFString[6];
char dpString[6];
char humidityString[6];
char pressureString[7];
char pressureInchString[6];


// only runs once on boot
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  delay(10);

  //set led pin as output
  pinMode(TICK_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  pinMode(MOISTURESENSOR_VCC,OUTPUT);
  
  // Connecting to WiFi network
  connectWifi();
  
  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
  Serial.println(F("BME280 test"));
  sensor.settings.commInterface = I2C_MODE; 
  sensor.settings.I2CAddress = 0x76;
  sensor.settings.runMode = 3; 
  sensor.settings.tStandby = 0;
  sensor.settings.filter = 0;
  sensor.settings.tempOverSample = 1 ;
  sensor.settings.pressOverSample = 1;
  sensor.settings.humidOverSample = 1;

  if (!sensor.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  float temperature = sensor.readTempC();
  float pressure=sensor.readFloatPressure()/100;
  float humidity=sensor.readFloatHumidity();

  digitalWrite(MOISTURESENSOR_VCC,HIGH);
  delay(800);
  moisture = analogRead(A0);
  delay(200);
  digitalWrite(MOISTURESENSOR_VCC,LOW);

  light=analogRead(LIGHTSENSOR);
  
  Serial.print("Température: ");
  Serial.print(temperature, 2);
  Serial.print(" °C");
  Serial.print("\t Pression: ");
  Serial.print(pressure, 2);
  Serial.print(" hPa");
  Serial.print("\t humidité relative : ");
  Serial.print(humidity, 2);
  Serial.println(" %");

  Serial.print("Humidité du sol : ");
  Serial.println(moisture);
  Serial.print("Luminosité : ");
  Serial.println(light);
  
  sendWeather(temperature, pressure, humidity,moisture,light);
  int count = myPeriodic;
  while(count--)
    delay(1000);
}

void sendWeather(float temp, float pressure, float humid, float moisture,int light) {  
   WiFiClient client;
  
   if (client.connect(server, 80)) {
     Serial.println("WiFi Client connected ");
     
     String postStr = apiKey;
     postStr += "&field1=";
     postStr += String(temp);
     postStr += "&field2=";
     postStr += String(pressure);
     postStr += "&field3=";
     postStr += String(humid);
     postStr += "&field4=";
     postStr += String(moisture);
     postStr += "&field5=";
     postStr += String(light);
     postStr += "\r\n\r\n";
     
     client.print("POST /update HTTP/1.1\n");
     client.print("Host: api.thingspeak.com\n");
     client.print("Connection: close\n");
     client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(postStr.length());
     client.print("\n\n");
     client.print(postStr);
     delay(1000);   
   }//end if
   sent++;
 client.stop();
}//end send

void connectWifi() {
    //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(TICK_LED, LOW);

}//end connect

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
    //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void tick() {
  //toggle state
  int state = digitalRead(TICK_LED);  // get the current state of GPIO1 pin
  digitalWrite(TICK_LED, !state);     // set pin to the opposite state
}

/*
void getWeather() {
  
    h = bme.readHumidity();
    t = bme.readTemperature();
    t = t*1.8+32.0;
    dp = t-0.36*(100.0-h);
    
    p = bme.readPressure()/100.0F;
    pin = 0.02953*p;
    dtostrf(t, 5, 1, temperatureFString);
    dtostrf(h, 5, 1, humidityString);
    dtostrf(p, 6, 1, pressureString);
    dtostrf(pin, 5, 2, pressureInchString);
    dtostrf(dp, 5, 1, dpString);
    delay(100);
 
}

// runs over and over again
void loop() {
  // Listenning for new clients
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New client");
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n' && blank_line) {
            getWeather();
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            // your actual web page that displays temperature
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<head><META HTTP-EQUIV=\"refresh\" CONTENT=\"15\"></head>");
            client.println("<body><h1>ESP8266 Weather Web Server</h1>");
            client.println("<table border=\"2\" width=\"456\" cellpadding=\"10\"><tbody><tr><td>");
            client.println("<h3>Temperature = ");
            client.println(temperatureFString);
            client.println("&deg;F</h3><h3>Humidity = ");
            client.println(humidityString);
            client.println("%</h3><h3>Approx. Dew Point = ");
            client.println(dpString);
            client.println("&deg;F</h3><h3>Pressure = ");
            client.println(pressureString);
            client.println("hPa (");
            client.println(pressureInchString);
            client.println("Inch)</h3></td></tr></tbody></table></body></html>");  
            break;
        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }  
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
} 
*/
