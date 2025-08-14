/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-plot-chart-web-server/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <LittleFS.h>
#else
  #include <Arduino.h>P
  #include <ESP8266WiFi.h>
  #include <Hash.h>
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h> 
  #include <LittleFS.h>
  #include <FS.h>
#endif
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>   

//------------- Sensor definition ---------------
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN     3      // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT22 // DHT 22 (AM2302), change to DHT11, if you using DHT11. 

// LED staue controlling pins 
#define RED_LED       17    // To control red led
#define GREEN_LED     18     // To control green led
#define YELLOW_LED    8    // To control yellow led

// define dht senser reading variable. 
DHT dht(DHTPIN, DHTTYPE);

// Replace with your network credentials
const char* ssid     = "VM1080293";
const char* password = "Omidmuhsin2015";

// number of the connected clients 
static int clients  = 0; 


// REPLACE with your Domain name and URL path or IP address with path
const char* serverName = "https://example.com/post-esp-data.php";

// Keep this API Key value to be compatible with the PHP code provided in the project page. 
// If you change the apiKeyValue value, the PHP file /post-esp-data.php also needs to have the same key 
String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorName = "DHT22";
String sensorLocation = "Office";

// web server listening on port 80 and websocket listening on port 81. 
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

// array to store sensor readings 
const int ARRAY_LENGTH = 10;
float temperature_vals[ARRAY_LENGTH];
float temperature_vals2[ARRAY_LENGTH];
float humidity_vals[ARRAY_LENGTH];

// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;                                  // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;                     // we use the "millis()" command for time reference and this will output an unsigned long


// Simple function to send information to the web clients(a single string) 
void sendJson(String l_type, String l_value) {
    String jsonString = "";                           // create a JSON string for sending data to the client
    StaticJsonDocument<200> doc;                      // create JSON container
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["type"] = l_type;                          // write data into the JSON object
    object["value"] = l_value;
    serializeJson(doc, jsonString);                   // convert JSON object to string
    webSocket.broadcastTXT(jsonString);               // send JSON string to all clients
}

// Simple function to send information to the web clients (arrays)
void sendJsonArray(String l_type, float l_array_values[], float updatedValue) {
    String jsonString = "";                           // create a JSON string for sending data to the client
    const size_t CAPACITY = JSON_ARRAY_SIZE(ARRAY_LENGTH) + 100;
    StaticJsonDocument<CAPACITY> doc;                      // create JSON container
    
    JsonObject object = doc.to<JsonObject>();         // create a JSON Object
    object["type"] = l_type;  
    object["updatedVal"] = updatedValue;                          // write data into the JSON object
    JsonArray value = object.createNestedArray("value");
    for(int i=0; i<ARRAY_LENGTH; i++) {
      value.add(l_array_values[i]);
    }
    serializeJson(doc, jsonString);                   // convert JSON object to string
    webSocket.broadcastTXT(jsonString);               // send JSON string to all clients
}


/* 
    Set-up function 
*/
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);  
   
   // leds initilaization 
  pinMode(GREEN_LED,OUTPUT);
  pinMode(RED_LED,OUTPUT);
  pinMode(YELLOW_LED,OUTPUT);
  // turning off leds 
  digitalWrite(GREEN_LED,LOW);
  digitalWrite(RED_LED,LOW);
  digitalWrite(YELLOW_LED,LOW);

  // Initialize LittleFS
  if(!LittleFS.begin())
  {
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
  }

  // --------- setup sensor ------------
    dht.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/webpage2.html");
  });
  
  server.serveStatic("/",LittleFS, "/");  
  webSocket.begin();                                 
  // start websocket
  webSocket.onEvent(webSocketEvent);    
  // Start server
  server.begin();
}
 
 
void loop() 
{
    webSocket.loop();   // Update function for the webSockets  
    unsigned long now = millis();   // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
    if ((unsigned long)(now - previousMillis) > interval) 
      { 
        // check if "interval" ms has passed since last time the clients were updated
        previousMillis = now;                             // reset previousMillis
        for(int i=0; i < ARRAY_LENGTH - 1; i++) 
        {
          temperature_vals[i] = temperature_vals[i+1];
          temperature_vals2[i] = temperature_vals2[i+1];
          humidity_vals[i]    = humidity_vals[i+1];
        }
    
      // Print temperature sensor details.
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      // Read temperature as Fahrenheit (isFahrenheit = true)
      float f = dht.readTemperature(true);

      // Check if any reading failed and exit early (to try again).
      if (isnan(h) || isnan(t) || isnan(f)) 
      {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
      }

      Serial.print(F("Humidity: "));
      Serial.print(h);      
      Serial.print("% ");

      Serial.print("Temperature: ");
      Serial.print(t);
      Serial.print(F("C"));

      Serial.print("Temperature: ");
      Serial.print(f);
      Serial.println(F("F "));
      
      temperature_vals[ARRAY_LENGTH - 1]  = t;
      temperature_vals2[ARRAY_LENGTH - 1] = f;
      humidity_vals[ARRAY_LENGTH - 1]     = h;
      // send data to the connected clients.
      sendJsonArray("temperature_update_c", temperature_vals,t);
      sendJsonArray("temperature_update_f", temperature_vals2,f);      
      sendJsonArray("humidity_update", humidity_vals,h);
    } 

    startTrafficLight();
     delay(2000);
}


/**
    When data is received from conencted clients, this function is excuted. 
*/
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) 
{      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) 
  {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:   
    {                      // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      clients--;
    }
    break;

    case WStype_CONNECTED:     
    {                       // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");   
      clients++;  
    }
    break;

    case WStype_TEXT:  
    {                               // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      StaticJsonDocument<200> doc;                    // create JSON container 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
         Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved
        // do here what needs to be done when server receive data from clients. 
        // the required actions are recieved from the clients, then do as needed. 
        int led_action = doc["action"];
        Serial.println("Request received from ESP32 " + String(led_action));         
        switch(led_action) 
        {
          case 1:
            digitalWrite(GREEN_LED,HIGH); // turn the green led on.
            break;
          case 2: 
            digitalWrite(GREEN_LED,LOW); // turn the green led off.
            break;
          case 3:
            digitalWrite(RED_LED,HIGH); // turn the red led on.
            break;
          case 4:
            digitalWrite(RED_LED,LOW); // turn the red led off.
            break;
          default: 
            Serial.println("Unknown Action, try again .... "); 
            break;
        }
      } 
    } break;
  }
}

void startTrafficLight()
{

    digitalWrite(GREEN_LED,HIGH); // turn the green led on.
    delay(5000); //wait 5 seconds 
    digitalWrite(GREEN_LED,LOW);    // turn the green led off.
    digitalWrite(YELLOW_LED,HIGH); // turn the yellow led on.
    delay(2000);  // wait 2 seconds.
    digitalWrite(YELLOW_LED,LOW); // turn the yellow led off.
    digitalWrite(RED_LED,HIGH); // turn the red led on.
    delay(5000); // wait 5 seconds. 
    digitalWrite(RED_LED,LOW); // turn the red led off.
}
