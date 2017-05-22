/************************************************************
Arduino Water Sensor
by Derek Lawless (https://github.com/flawless2011/arduino-water-sensor
************************************************************/

//////////////////////
// Library Includes //
//////////////////////
// SoftwareSerial is required (even you don't intend on
// using it).
#include <SoftwareSerial.h> 
#include <SparkFunESP8266WiFi.h>
#include <AES.h>
#include <Base64.h>

//////////////////////////////
// WiFi Network Definitions //
//////////////////////////////
// Replace these two character strings with the name and
// password of your WiFi network.
const char mySSID[] = "gondor-guest";
const char myPSK[] = "lawfulguests11";

//////////////////
// HTTP Strings //
//////////////////

// TODO add real hostname here for node.js server
const char destServer[] = "gandalf";

// TODO add real phone number here
const char jsonString[] = "{\"toNumber\":\"\"}";

const int sensorPin = A0;

const String httpRequestT = "POST /api/alerts HTTP/1.1\r\n"
                            "Host: gandalf\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: ";

int waterDetected = 0;
int contentLength = 0;

// All functions called from setup() are defined below th/e
// loop() function. They modularized to make it easier to
// copy/paste into sketches of your own.
void setup() 
{
  // Serial Monitor is used to control the demo and view
  // debug information.
  Serial.begin(9600);

  // initializeESP8266() verifies communication with the WiFi
  // shield, and sets it up.
  initializeESP8266();

  // connectESP8266() connects to the defined WiFi network.
  connectESP8266();

  // displayConnectInfo prints the Shield's local IP
  // and the network it's connected to.
  displayConnectInfo();

  // Setup water sensor
  pinMode(sensorPin, INPUT);
}

void loop() 
{
  // Check the sensor voltage
  if (digitalRead(sensorPin) == HIGH) {
    Serial.println(F("A0: No Water"));
    waterDetected = 0;
  }
  else {
    Serial.println(F("A0: Water!!"));
    // Only send a message if we've detected water 5 times in a row
    if (waterDetected++ >= 5) {
      waterDetected = 0;
      sendMessage(encryptMessage());
      // Wait 30 minutes before sending another message
      Serial.println(F("Sleeping for 30 minutes..."));
      delay(60000 * 30);
    }
  }
  delay(6000);
}

char* encryptMessage() {
  // Encrypt JSON
  AES aes;
  byte cipher [2*N_BLOCK];
  // TODO add real key here
  byte key[] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  byte my_iv[] =
  {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
  };
  byte iv [N_BLOCK];
  byte succ = aes.set_key (key, 256);
  for (byte i = 0 ; i < 16 ; i++) {
      iv[i] = my_iv[i];
  }
  succ = aes.cbc_encrypt((byte*)jsonString, cipher, 2, iv);

  // Base64 encode JSON
  int inputLen = sizeof(cipher);
  contentLength = base64_enc_len(inputLen);
  char encoded[contentLength];

  // note input is consumed in this step: it will be empty afterwards
  base64_encode(encoded, cipher, inputLen);
  return encoded;
}

void sendMessage(char* encoded) {
  String json(encoded);
  String httpRequest = "" + httpRequestT;
  httpRequest += contentLength + 14;
  httpRequest += "\r\n\r\n{\"payload\":\"";
  httpRequest += json;
  httpRequest += "\"}";
  // Attach to httpRequest String
  // Send
  makeHttpCall(httpRequest);
}

void makeHttpCall(String httpRequest)
{
  // To use the ESP8266 as a TCP client, use the
  // ESP8266Client class. First, create an object:
  ESP8266Client client;

  // ESP8266Client connect([server], [port]) is used to
  // connect to a server (const char * or IPAddress) on
  // a specified port.
  // Returns: 1 on success, 2 on already connected,
  // negative on fail (-1=TIMEOUT, -3=FAIL).
  int retVal = client.connect(destServer, 8080);
  if (retVal <= 0)
  {
    Serial.print(F("Failed to connect to server. retVal="));
    Serial.println(retVal);
    return;
  }
  // print and write can be used to send data to a connected
  // client connection.
  client.print(httpRequest);

  // available() will return the number of characters
  // currently in the receive buffer.
  while (client.available())
    Serial.write(client.read()); // read() gets the FIFO char

  // connected() is a boolean return value - 1 if the
  // connection is active, 0 if it's closed.
  if (client.connected())
    client.stop(); // stop() closes a TCP connection.
}

void initializeESP8266()
{
  // esp8266.begin() verifies that the ESP8266 is operational
  // and sets it up for the rest of the sketch.
  // It returns either true or false -- indicating whether
  // communication was successul or not.
  // true
  int test = esp8266.begin();
  if (test != true)
  {
    Serial.println(F("Error talking to ESP8266."));
    errorLoop(test);
  }
  Serial.println(F("ESP8266 Shield Present"));
}

void connectESP8266()
{
  // The ESP8266 can be set to one of three modes:
  //  1 - ESP8266_MODE_STA - Station only
  //  2 - ESP8266_MODE_AP - Access point only
  //  3 - ESP8266_MODE_STAAP - Station/AP combo
  // Use esp8266.getMode() to check which mode it's in:
  int retVal = esp8266.getMode();
  if (retVal != ESP8266_MODE_STA)
  { // If it's not in station mode.
    // Use esp8266.setMode([mode]) to set it to a specified
    // mode.
    retVal = esp8266.setMode(ESP8266_MODE_STA);
    if (retVal < 0)
    {
      Serial.println(F("Error setting mode."));
      errorLoop(retVal);
    }
  }
  Serial.println(F("Mode set to station"));

  // esp8266.status() indicates the ESP8266's WiFi connect
  // status.
  // A return value of 1 indicates the device is already
  // connected. 0 indicates disconnected. (Negative values
  // equate to communication errors.)
  retVal = esp8266.status();
  if (retVal <= 0)
  {
    Serial.print(F("Connecting to "));
    Serial.println(mySSID);
    // esp8266.connect([ssid], [psk]) connects the ESP8266
    // to a network.
    // On success the connect function returns a value >0
    // On fail, the function will either return:
    //  -1: TIMEOUT - The library has a set 30s timeout
    //  -3: FAIL - Couldn't connect to network.
    retVal = esp8266.connect(mySSID, myPSK);
    if (retVal < 0)
    {
      Serial.println(F("Error connecting"));
      errorLoop(retVal);
    }
  }
}

void displayConnectInfo()
{
  char connectedSSID[24];
  memset(connectedSSID, 0, 24);
  // esp8266.getAP() can be used to check which AP the
  // ESP8266 is connected to. It returns an error code.
  // The connected AP is returned by reference as a parameter.
  int retVal = esp8266.getAP(connectedSSID);
  if (retVal > 0)
  {
    Serial.print(F("Connected to: "));
    Serial.println(connectedSSID);
  }

  // esp8266.localIP returns an IPAddress variable with the
  // ESP8266's current local IP address.
  IPAddress myIP = esp8266.localIP();
  Serial.print(F("My IP: ")); Serial.println(myIP);
}

// errorLoop prints an error code, then loops forever.
void errorLoop(int error)
{
  Serial.print(F("Error: ")); Serial.println(error);
  Serial.println(F("Looping forever."));
  for (;;)
    ;
}
