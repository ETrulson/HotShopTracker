#include <Adafruit_Si7021.h>
#include <Adafruit_MAX31855.h>
#include <SPI.h>
#include "Auber.h"
#include <WiFiNINA.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/////////////////////////// Initialize variables //////////////////////////////////////////////////////
char ssid[] = "Formlabs Guest";         // your network SSID (name)
char pass[] = "formguest";         // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;
char server[] = "io.adafruit.com"; // name address for Adafruit IOT Cloud

float _PID_Data_Process_Value = 0;
float _PID_Data_Set_Value = 0;
float _Type_K_Thermocouple_Temp = 0;
float _Current_Transformer = 0;
float _Ambient_Sensor_Temperature = 0;
float _Ambient_Sensor_Humidity = 0;

unsigned long lastConnectionTime = 0;              // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5000;       // delay between updates, in milliseconds

///////////////////////////// Initialize client libraries & sensor objects /////////////////////////////
WiFiClient client;
Auber auber;
Adafruit_Si7021 sensor = Adafruit_Si7021();

// Construct the driver for the thermocouple. It will use the SPI bus pins, in
// addition to your choice of chip-select (CS) pin.
// On the MKR WiFi1010 board, those are:
//    MISO: 10
//    SCK:   9
const uint8_t THERMOCOUPLE_CS_PIN = 0;
Adafruit_MAX31855 thermocouple(THERMOCOUPLE_CS_PIN);

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883 // use 8883 for SSL
#define AIO_USERNAME    "ETrulson"
#define AIO_KEY "9ef249e26b7a4409a97d87560f17ed7c"
#define AIO_GROUP       "hotshoptracker"

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish feed_thermocouple = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" AIO_GROUP ".type-k-thermocouple");
Adafruit_MQTT_Publish feed_process_value = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" AIO_GROUP ".pid-data-process-value");
Adafruit_MQTT_Publish feed_set_value = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/" AIO_GROUP ".pid-data-set-value");

/////////////////////////// Main Setup function ////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  //while (!Serial); // wait for serial port to connect. Needed for native USB port only

  //initialize Wifi connection and auber connection TODO: Add success & error messages?
  connectToWIFI();
  auber.setup();

  //initialize Si7021 sensor connection, return an error if it's not found
  if (sensor.begin()) {
    Serial.println("Connected to Si7021 sensor");
  } else if (!sensor.begin()) {
    Serial.println("Error! Did not find Si7021 sensor");
  }
}

////////////////////////// Main Loop Function ///////////////////////////////////////////////////////////
void loop() {
  // if 10 seconds have passed since your last connection, then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
      readSensors(); // read sensors
      displayValuesOnSerial(); //display sensor values on serial
      connectMQTT();
      publishMQTT();
    }
}

// Read all sensor values
void readSensors() {
  //PID Process Value (Temperature)
  TempReading process = auber.getProcessTemp();
  _PID_Data_Process_Value = process.value;
  if (!process.ok) {
    Serial.println("error! process not ok");
  }

  //PID Set Value (Temperature)
  TempReading setpoint = auber.getSetpointTemp();
  _PID_Data_Set_Value = setpoint.value;
  if (!setpoint.ok) {
    Serial.println("error! setpoint not ok");
  }

  //Type K TC (Temperature)
  _Type_K_Thermocouple_Temp = thermocouple.readCelsius();

  //*STATIC PLACEHOLDER VALUE* Current Transformer (Current)
  _Current_Transformer = 45; //ENV.readLux();

  //Si7021 Ambient Sensor (Temperature)
  _Ambient_Sensor_Temperature = sensor.readTemperature();

  //Si7021 Ambient Sensor (Humidity)
  _Ambient_Sensor_Humidity = sensor.readHumidity();
}

// Display values on Serial Port
void displayValuesOnSerial() {
  Serial.println();
    
  Serial.print("PID Process Value = ");
  Serial.print(_PID_Data_Process_Value);
  Serial.println("°F");

  Serial.print("PID Set Value = ");
  Serial.print(_PID_Data_Set_Value);
  Serial.println(" °F");

  Serial.print("Type K TC Value = ");
  Serial.print(_Type_K_Thermocouple_Temp);
  Serial.println(" °F");

  Serial.print("Current Transformer Value = ");
  Serial.println(_Current_Transformer);
  Serial.println(" amps");

  Serial.print("Ambient Shop Temperature = ");
  Serial.print(_Ambient_Sensor_Temperature);
  Serial.println(" °C");

  Serial.print("Ambient Shop Humidity = ");
  Serial.print(_Ambient_Sensor_Humidity);
  Serial.println(" %");
  
  Serial.println();
}

void publishMQTT() {
  // TODO check if they were published succesfully? what would we do if they weren't?
  feed_process_value.publish(_PID_Data_Process_Value);
  feed_set_value.publish(_PID_Data_Set_Value);
  feed_thermocouple.publish(_Type_K_Thermocouple_Temp);

  // note the time that the connection was made:
  lastConnectionTime = millis();
}

// Function to connect and reconnect as necessary to the MQTT server.
void connectMQTT() {
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  int8_t error_code;
  while ((error_code = mqtt.connect()) != 0) { // will return 0 if successfull
    Serial.println(mqtt.connectErrorString(error_code));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);
  }
  Serial.println("MQTT connected!");
}

void connectToWIFI() {
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

