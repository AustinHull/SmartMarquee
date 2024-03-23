#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// #include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <Max72xxPanel.h>
#include "LittleFS.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include <EEPROM.h>
#include "Adafruit_HT1632.h"
#include "Adafruit_GFX.h"
#include "Fonts/FreeSans9pt7b.h"

#define ESP8266_LED 5

//////////////////////
// WiFi Definitions //
//////////////////////
char* WiFiSSID = new char[32];
char* WiFiPSK = new char[64];
char WiFiAPPSK[] = "sparkfun";
bool isConfigured = false;
bool isConnectedToHostAP = false;
bool mDNSActive = false;

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read
const int MATRIX_DATA_PIN = 2;
const int MATRIX_CS0_PIN = 4;
const int MATRIX_WRITE_PIN = 0;
const int MATRIX_CS1_PIN = 13;

// data source-structure to write from.
struct {
  char saved[9] = "notSaved";
  char idStr[33] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  char pwrdStr[65] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
} apConfig;

// Filesystem items should be used for handling relatively static but "heavy" items, such as images as well as HTML base template files (should be populated with data separately, likely via JS scripts

AsyncWebServer server(80);

// Begin instantiation for Dot-Matrix LED Displays (uses Adafruit HT1632 multi-displays)
Adafruit_HT1632LEDMatrix matrix = Adafruit_HT1632LEDMatrix(MATRIX_DATA_PIN, MATRIX_WRITE_PIN, MATRIX_CS0_PIN, MATRIX_CS1_PIN);

void setup() 
{
  initHardware();
  // Set WiFi mode to station-accesspoint
  WiFi.mode(WIFI_AP_STA);
  // Check our EEPROM storage for previously-saved network config values prior to proceeding with our chosen launch flow states.
  setupWiFi(); // We start with setting up our own local network first-and-foremost...
  configureWifiBridgePage(); // Then configuring our local server to work in a User Configuration mode until connection has been established with an AP of the user.
  server.serveStatic("/", LittleFS, "/www").setDefaultFile("index.html");
  server.begin();
  setupMDNS();
}

void loop() 
{
  // Called to keep local mDNS instance running for users on the intranet.
  if (mDNSActive) {
    MDNS.update();
  }

  if (isConfigured && !isConnectedToHostAP) {
    connectWiFi();
  } else if (!isConfigured && !isConnectedToHostAP) {
    isConfigured = getDoesConfigExist();
  }
}
void configureWifiBridgePage() {
  // Send web page with input fields to client
  //WiFi.scanNetworks(true); // <-- THIS call must be done outside of async server handlers such as the one below - heed this warning lest ye enjoy soft-crashing your device AP repeatedly and often! Limitation is currently that this only makes one scan on device startup, can be resolved by refactors later.
  server.on("/configureHomeNetConnect.json", HTTP_GET, [](AsyncWebServerRequest *request){
    int n = WiFi.scanComplete();
    // Prepare the response, starting with common SUCCESS header
    String output;
		output = "{\"networks\":{\"ssids\":\[";
		char 	ssid_buf[100];
    // int n = -3; // Just a throwaway initializer so we know it hasn't been touched by scan processes yet.
    /* while( n < 0 && (n = WiFi.scanComplete()) < 0) {
      delay(1000); // We need to use this asynchronous scan-delay loopback until the scan is completed. The synchronous version can scan long enough that it blocks critical device operations, causing a crash-reset during page loads.
      // Serial.println("Scan in progress");
    } */
    if(n == -2){
      WiFi.scanNetworks(true);
    } else if (n == 0) {
      snprintf(ssid_buf, sizeof(ssid_buf), " {\"ssid\":\"%s\",\"encryptionType\":\"%d\",\"rssi\":\"-%d\"},", "none", 255, 0);
      Serial.println("Sending empty JSON response now");
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    } else {
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found and store it in our pointers:
        snprintf(ssid_buf, sizeof(ssid_buf), " {\"ssid\":\"%s\",\"encryptionType\":\"%d\",\"rssi\":\"%d\"}", WiFi.SSID(i).c_str(), WiFi.encryptionType(i), WiFi.RSSI(i));
        output.concat(ssid_buf);
        if (i < n - 1) {
          output.concat(",\n");
        } else {
          output.concat("\n");
        }
      }
      WiFi.scanDelete();
      if(WiFi.scanComplete() == -2){
        WiFi.scanNetworks(true);
      }
    };
    output.concat("]}}");
    Serial.println("Sending JSON response now.");
    request->send(200, "application/json", output);

    // Maybe try and implement Dot-Matrix one-off print for when we successfully connect to a network OR served HTML page.Adafruit HT1632
  });

  server.on("/signInToHomeNet.json", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    // By traversing and copying over data from the body per-byte, we ensure we're only copying over the amount of data expected by the request itself; helps keep extra garbage from being picked up after the good data under some circumstances.
    char dataBuf[len];
    for (size_t i = 0; i < len; i++) {
      dataBuf[i] = data[i];
    }
    const size_t CAPACITY = JSON_OBJECT_SIZE(16);
    StaticJsonDocument<CAPACITY> doc;
    deserializeJson(doc, dataBuf);
    JsonObject jsonObj = doc.as<JsonObject>();
    if (jsonObj.containsKey("chosenSsid")) {
      if (jsonObj.containsKey("enteredPassword")) {
        // Store AP credentials for local reference, then attempt connection and verify success.
        String ssid = jsonObj["chosenSsid"];
        String password = jsonObj["enteredPassword"];
        const char* charSsid = ssid.c_str();
        const char* charPassword = password.c_str();
        strcpy(WiFiSSID, charSsid);
        strcpy(WiFiPSK, charPassword);
        String requestJson = String((char*)WiFiSSID);
        Serial.println("JSON params received from client-user: " + requestJson);
        request->send(200,"text/json","{\"status\":\"success\"}");
        isConfigured = true;
        // commit 512 bytes of ESP8266 flash (for "EEPROM" emulation)
        // this step actually loads the content (512 bytes) of flash into 
        // a 512-byte-array cache in RAM
        uint addr = 0;
        strcpy(apConfig.saved, "xxxSaved");
        strcpy(apConfig.idStr, charSsid);
        strcpy(apConfig.pwrdStr, charPassword);
        EEPROM.put(addr, apConfig);
        EEPROM.commit();
      } else {
        request->send(500,"text/json","{\"status\":\"Missing Parameters (AP Password).\"}");
      }
    } else {
      request->send(500,"text/json","{\"status\":\"Missing Parameters (AP SSID).\"}");
    }
  });
}

bool getDoesConfigExist() {
  uint addr = 0;
  // read bytes (i.e. sizeof(data) from "EEPROM"),
  // in reality, reads from byte-array cache
  // cast bytes into structure called data
  EEPROM.get(addr, apConfig);

  if (strcmp(apConfig.saved, "xxxSaved") == 0) {
    strcpy(WiFiSSID, apConfig.idStr);
    strcpy(WiFiPSK, apConfig.pwrdStr);
    Serial.println("Test to: " + String(apConfig.idStr));
    return true;
  } else {
    return false;
  }
}

void connectWiFi() {
  byte ledStatus = LOW;
  Serial.println();
  Serial.println("Connecting to: " + String(WiFiSSID));

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED) {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  digitalWrite(LED_PIN, LOW); // Signify that we now have an established STAtion connection to user's local network.
  isConnectedToHostAP = true;
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // TODO-?: At this point, we can invoke some commands to convert STA + AP over to STA mode for further, relatively-seamless access via their local (AP) router network.

  // Matrix display update to let user know of wifi connection?
  String wifiConnectString = "WiFi Connected!";
  int16_t textX = 0;
  for (int offset = 0; offset < wifiConnectString.length() * 6; offset++) {
    matrix.clearScreen();
    matrix.setCursor(textX, 4);
    matrix.print(wifiConnectString);
    matrix.writeScreen();
    // Move text left (w/wrap), increase hue
    --textX;
    delay(200);
  }
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin("marquee")) 
  {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  mDNSActive = true;
  Serial.println("mDNS responder started");

}

void initHardware()
{
  Serial.begin(115200);
  matrix.begin(ADA_HT1632_COMMON_16NMOS);
  matrix.clearScreen();
  matrix.setTextWrap(false);
  matrix.setRotation(0);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.

  // LittleFSConfig fsConfig;
  // LittleFS.setConfig(fsConfig);
  // uint8_t maxfiles = 255;
  LittleFS.begin();
  EEPROM.begin(sizeof(apConfig));
  // LittleFS.begin();
}

// Initializes the device-local Access Point for user to connect their own home network devices to in order to configure this device itself.
void setupWiFi() {
  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "ThingDev-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "SmartMarquee-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}
