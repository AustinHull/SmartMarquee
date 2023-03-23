#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
// #include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// #include <Max72xxPanel.h>
#include "LittleFS.h"

#define ESP8266_LED 5

//////////////////////
// WiFi Definitions //
//////////////////////
char WiFiSSID[] = {};
char WiFiPSK[] = {};
char WiFiAPPSK[] = "sparkfun";
bool isConfigured = false;

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read

// Filesystem items should be used for handling relatively static but "heavy" items, such as images as well as HTML base template files (should be populated with data separately, likely via JS scripts

AsyncWebServer server(80);

void setup() 
{
  initHardware();
  // Set WiFi mode to station-accesspoint
  WiFi.mode(WIFI_AP_STA);
  // Serial.printf("isConfigured=%s\n", isConfigured);
  if (WiFiSSID == NULL) {
    isConfigured = true;
    connectWiFi(); // Connect to local AP first if we have credentials stored
    setupWiFi();
  } else {
    setupWiFi(); // If we don't have an existing AP to connect to first, we start with setting up our own local network first-and-foremost...
    configureWifiBridgePage(); // Then configuring our local server to work in a User Configuration mode until connection has been established with an AP of the user.
  }
  server.serveStatic("/", LittleFS, "/www").setDefaultFile("index.html");
  server.begin();
  setupMDNS();
}

void loop() 
{
  // server.handleClient();
  // Check if a client has been connected.
  // WiFiClient client = server.accept();
  /*if (!client) {
    return;
  }*/

  int networksScanned = -2;
  if (/*(networksScanned = WiFi.scanComplete()) >= 0 && */!isConfigured) {
    // Read the furst line of the request.
    // String req = client.readStringUntil('\r');
    // Serial.println(req);
    // client.flush();

    // Prepare the response, starting with common SUCCESS header
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n\r\n";
    response += "<!DOCTYPE HTML>\r\n<html>\r\n";

    /*Note: Uncomment the line below to refresh automatically
    *      for every 1 second. This is not ideal for large pages 
    *      but for a simple read out, it is useful for monitoring 
    *      your sensors and I/O pins. To adjust the fresh rate, 
    *      adjust the value for content. For 30 seconds, simply 
    *      change the value to 30.*/
    // response += "<meta http-equiv='refresh' content='1'/>\r\n";  //auto refresh page

    // IF/once we have a list of available networks, send a response back to the user that show them their options.
    // TODO: Sort responses by SSID Radio Strength, properly format encryption/security information for user readability, and implement log-in authentication flow for user's home network configurations.
    if (networksScanned > 0) {
      response += "SmartMarquee's Available Networks:";
      response += "<br>";  // Go to the next line.
      response += "<table><thead><tr><th colspan='2'>SSID</th><th colspan='2'>Strength</th><th colspan='2'>Security</th><th colspan='2'>Select?</th></tr></thead><tbody>";
      for (int i = 0; i < networksScanned; i++) {
        // Hardware-supported firmware is capable of connecting to 2.4G 802.11 WiFi only, capable on connecting to unsecured, WEP, TKIP (WPA / PSK), CCMP (WPA2 / PSK), and 'Auto' (WPA / WPA2 / PSK) modes compatible with the aforementioned mode details. Any other encryptionTypes returned by Access Points retrieved will return 255, denoting an Unsupported status.
        uint8_t encryptionType = WiFi.encryptionType(i);
        int32_t signalStrengthValue = WiFi.RSSI(i); // TODO: Integrate this into a GUI format allowing users to see relative signal integrity as a visual indicator (eg, "Signal Wave-bars filled"), without having to read or interpret a numerical RSSID value.
        response += "<tr><td colspan='2'>" +  WiFi.SSID(i) + "</td><td colspan='2'>" + signalStrengthValue + "</td><td colspan='2'>" + (encryptionType == ENC_TYPE_NONE ? "Unsecured" : (encryptionType == ENC_TYPE_AUTO ? "Auto(WPA/WPA2)" : (encryptionType == ENC_TYPE_WEP ? "WEP" :  (encryptionType == ENC_TYPE_TKIP ? "WPA" :  (encryptionType == ENC_TYPE_CCMP ? "WPA2" :  "Unsupported"))))) + "</td><td colspan='2'><input type='submit'></td></tr>";
      }
    } else {
       response += "SmartMarquee Cannot Find Any Available Compatible Networks";
    }
    response += "</tbody></table></html>\n";

    // Send the response to the client
    // client.print(response);
    delay(100);
    // Serial.println("Client Disconnected");
    return;
  }

  // Read the furst line of the request.
  // String req = client.readStringUntil('\r');
  // Serial.println(req);
  // client.flush();

  //Match the request
  /* int val = -1; // We'll use 'val' to keep track of both the
                // request type (read/set) and value if set.
  if (req.indexOf("/led/0") != -1) {
    val = 1; // Will write LED high
  } else if (req.indexOf("/led/1") != -1) {
    val = 0;
  } else if (req.indexOf("/read") != -1) {
    val = -2; // Will print pin reads
              // Otherwise request will be invalid. We'll say as much in HTML
  } else {

  }

  // Set GPIOs according to the request
  if (val >= 0) {
    digitalWrite(LED_PIN, val);
  }

  client.flush();

  // Prepare the response, starting with common SUCCESS header
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: text/html\r\n\r\n";
  response += "<!DOCTYPE HTML>\r\n<html>\r\n";

  /*Note: Uncomment the line below to refresh automatically
   *      for every 1 second. This is not ideal for large pages 
   *      but for a simple read out, it is useful for monitoring 
   *      your sensors and I/O pins. To adjust the fresh rate, 
   *      adjust the value for content. For 30 seconds, simply 
   *      change the value to 30.*/
  /*response += "<meta http-equiv='refresh' content='1'/>\r\n"; //auto refresh page

  // Things to write if we set the LED:
  if (val >= 0) {
    response += "LED is now ";
    response += (val)?"off":"on";
  } else if (val == -2) {
    // If we're reading pins, print out those values:
    response += "Analog Pin = ";
    response += String(analogRead(ANALOG_PIN));
    response += "<br>"; // Go to the next line.
    response += "Digital Pin 12 = ";
    response += String(digitalRead(DIGITAL_PIN));
  } else {
    response += "Invalid Request.<br> Try /led/1, /led/0, or /read.";
  }
  response += "</html>\n";

  // Send the response to the client
  // client.print(response);
  delay(1);
  // Serial.println("Client Disconnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed */
}

void configureWifiBridgePage() {
  // Send web page with input fields to client
  WiFi.scanNetworks(true); // THIS is what's causing a Connection Reset/Crash!! due to the call blocking critical internal maintanence systems. ***TODO: USE ASYNC***
  server.on("/configureHomeNetConnect.json", HTTP_GET, [](AsyncWebServerRequest *request){
    // Prepare the response, starting with common SUCCESS header
    String output;
		output = "{\"networks\":{\"ssids\":\[";
		char 	ssid_buf[100];
    int n = -3; // Just a throwaway initializer so we know it hasn't been touched by scan processes yet.
    while( n < 0 && (n = WiFi.scanComplete()) < 0) {
      delay(500); // We need to use this asynchronous scan-delay loopback until the scan is completed. The synchronous version can scan long enough that it blocks critical device operations, causing a crash-reset during page loads.
    }
		if (n == 0) {
      snprintf(ssid_buf, sizeof(ssid_buf), " {\"ssid\":\"%s\",\"encryptionType\":\"%d\",\"rssi\":\"-%d\"},", "none", 255, 0);
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
    };
    output.concat("]}}");
    Serial.println("Sending JSON response now");
    request->send(200, "text/json", output);
  });
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
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin("thing")) 
  {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

}

void initHardware()
{
  Serial.begin(115200);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.

  // LittleFSConfig fsConfig;
  // LittleFS.setConfig(fsConfig);
  // uint8_t maxfiles = 255;
  LittleFS.begin();
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
  String AP_NameString = "ThingDev-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  WiFi.softAP(AP_NameChar, WiFiAPPSK);
}