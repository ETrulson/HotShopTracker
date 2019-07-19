/*#include <AdafruitIO.h>
#include <AdafruitIO_Dashboard.h>
#include <AdafruitIO_Data.h>
#include <AdafruitIO_Definitions.h>
#include <AdafruitIO_Ethernet.h>
#include <AdafruitIO_Feed.h>
#include <AdafruitIO_FONA.h>
#include <AdafruitIO_Group.h>
#include <AdafruitIO_MQTT.h>
#include <AdafruitIO_Time.h>
#include <AdafruitIO_WiFi.h>
*/

#include <ArduinoJson.h>  
#include <avr/dtostrf.h>
#include <SPI.h>

#include "Auber.h"


/////////////////////////
#include <WiFiNINA.h>
//#include "arduino_secrets.h" 

char ssid[] = "Formlabs Guest";         // your network SSID (name)
char pass[] = "formguest";         // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;
char server[] = "io.adafruit.com"; // name address for Adafruit IOT Cloud

///////////////////////////
float _PID_Data_Process_Value = 0;
float _PID_Data_Set_Value = 0;
float _Type_K_Thermocouple_Temp = 0;
float _Current_Transformer = 0;

unsigned long lastConnectionTime = 0;              // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5000;       // delay between updates, in milliseconds

// Initialize the client library
WiFiClient client;
Auber auber;

void setup() {
 
  Serial.begin(9600);
  //while (!Serial); // wait for serial port to connect. Needed for native USB port only
 
  connectToWIFI();
  auber.setup();
 
}

void loop() {

 // if 10 seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) 
  {
    readSensors(); // read sensors
    displayValuesOnSerial(); //display sensor values on serial
    httpRequest(); // send data to Cloud
  }
  
}


/* 
 * This method makes a HTTP connection to the server and post deread sensor values 
 * to the Adafruit IOT Cloud
 */

void httpRequest() 
{

  
/*
 * https://io.adafruit.com/api/docs/#operation/createGroupData
 * 
 * POST /{username}/groups/{group_key}/data
 * 
 * JSON:
 * 
{
  "location": {
    "lat": 0,
    "lon": 0,
    "ele": 0
  },
  "feeds": [
    {
      "key": "string",
      "value": "string"
    }
  ],
  "created_at": "string"
}
 */

  const size_t capacity = JSON_ARRAY_SIZE(3) + 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + 130;
  StaticJsonDocument<capacity> doc;

   // Add the "location" object
  JsonObject location = doc.createNestedObject("location");
  location["lat"] = 0;
  location["lon"] = 0;
  location["ele"] = 0;
  
  // Add the "feeds" array
  JsonArray feeds = doc.createNestedArray("feeds");
  JsonObject feed1 = feeds.createNestedObject();
  feed1["key"] = "pid-data-process-value";
  feed1["value"] = _PID_Data_Process_Value;
  JsonObject feed2 = feeds.createNestedObject();
  feed2["key"] = "pid-data-set-value";
  feed2["value"] = _PID_Data_Set_Value;
  //JsonObject feed3 = feeds.createNestedObject();
  //feed3["key"] = "Type K Temperature";
  //feed3["value"] = _Type_K_Thermocouple_Temp;
  //JsonObject feed4 = feeds.createNestedObject();
  //feed4["key"] = "Current Transformer Current";
  //feed4["value"] = _Current_Transformer;

  
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

char IO_USERNAME [] = "ETrulson";
char IO_GROUP [] = "hotshoptracker";
char IO_KEY [] = "9ef249e26b7a4409a97d87560f17ed7c";

  Serial.println("\nStarting connection to server...");
  if (client.connect(server, 80)) 
  {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.print("POST /api/v2/");
    client.print(IO_USERNAME);
    client.print("/groups/");
    client.print(IO_GROUP);
    client.println("/data HTTP/1.1");
     
    client.println("Host: io.adafruit.com");  
    client.println("Connection: close");  
    client.print("Content-Length: ");  
    client.println(measureJson(doc));  
    client.println("Content-Type: application/json");
      
    client.print("X-AIO-Key: ");
    client.println(IO_KEY); 

    // Terminate headers with a blank line
    client.println();
    // Send JSON document in body
    serializeJson(doc, client);

    // note the time that the connection was made:
    lastConnectionTime = millis();

// Display the HTTP request on the serial monitor:
Serial.println("HTTP requst: ");
    Serial.print("POST /api/v2/");
    Serial.print(IO_USERNAME);
    Serial.print("/groups/");
    Serial.print(IO_GROUP);
    Serial.println("/data HTTP/1.1");
     
    Serial.println("Host: io.adafruit.com");  
    Serial.println("Connection: close");  
    Serial.print("Content-Length: ");  
    Serial.println(measureJson(doc));  
    Serial.println("Content-Type: application/json");
      
    Serial.print("X-AIO-Key: ");
    Serial.println(IO_KEY); 
    Serial.println();

    
    Serial.println("JSON data: ");
    serializeJson(doc, Serial);
    Serial.println();
    Serial.println("data sent!");
    
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed!");
  }

  
}

void connectToWIFI()
{
   // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to wifi");
  delay(2000);
  printWifiStatus();
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//Read sensors value: Temperature, Humidity, Pressure, Lux
void readSensors()
{
  TempReading process = auber.getProcessTemp();
  _PID_Data_Process_Value = process.value;
  if (!process.ok) {
      Serial.println("error! process not ok");
  }

  TempReading setpoint = auber.getSetpointTemp();
  _PID_Data_Set_Value = setpoint.value;
  if (!setpoint.ok) {
    Serial.println("error! setpoint not ok");
  }

  _Type_K_Thermocouple_Temp = 44; //ENV.readPressure();
  _Current_Transformer = 45; //ENV.readLux();
}

// Display values on Serial Port
void displayValuesOnSerial()
{
  Serial.println();
    
  Serial.print("PID Process Value = ");
  Serial.print(_PID_Data_Process_Value);
  Serial.println("°F");

  Serial.print("PID Set Value = ");
  Serial.print(_PID_Data_Set_Value);
  Serial.println("°F");

  Serial.print("Type K TC Value = ");
  Serial.print(_Type_K_Thermocouple_Temp);
  Serial.println("°F");

  Serial.print("Current Transformer Value = ");
  Serial.println(_Current_Transformer);
  Serial.println("amps");

  Serial.println();

}
