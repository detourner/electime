#include "WifiManager.h"
#include "HardwareSerial.h"
#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "HCMS39xx.h"
#include "gauge_freq_meter.h"
#include <time.h>

// --------------------- CONFIGURATION ---------------------

const char* apSSID = "ElecTime";
const char* apPassword = "12345678";

// WebSocket server configuration
const char* serverName = "electime";  // Replace with your WebSocket server name
const int websocketPort = 8765;       // WebSocket server port

const float minFrequency = 49.80f; // Minimum valid frequency
const float maxFrequency = 50.20f; // Maximum valid frequency

// --------------------- GLOBAL VARIABLES ---------------------

WebSocketsClient webSocket;

// See https://github.com/Andy4495/HCMS39xx/blob/main/README.md#hardware-connections for wiring info
// HCMS39xx(uint8_t num_chars, uint8_t data_pin, uint8_t rs_pin, uint8_t clk_pin, 
//          uint8_t ce_pin, uint8_t blank_pin)
HCMS39xx display(8, D10, D2, D8, D0, D3); // osc_select_pin tied high, not connected to microcontroller
WifiManager wifiManager(apSSID, apPassword);

// GaugeFreqMeter(uint8_t pinStep, uint8_t pinDir, uint8_t pinReset)
GaugeFreqMeter gaugeFreqMeter;

// --------------------- UTILITY FUNCTIONS ---------------------

// Updates the display with the current time and frequency
// If needFreqUpdate is true, the frequency used to correct the displayed time is updated
// If needFreqUpdate is false, the displayed time is updated using the last known frequency
// Returns the elapsed time since the last update for the display
// The frequency is assumed to be in Hz and must be between 49.80 and 50.20 Hz
// The drift is calculated based on the frequency and applied to the current time
unsigned long updateDisplayWithCurrentTime(bool needFreqUpdate, float frequency) 
{
  static float lastFrequency = 50.0f;
  if(needFreqUpdate) 
  {
    lastFrequency = frequency; // Updates the last frequency
  }

  // Calculate the drift in seconds over a year
  float frequencyDeviation = lastFrequency - 50.0; // Difference from 50 Hz
  int secondsPerYear = (int)(frequencyDeviation * 365.0 * 60.0 * 60.0); // Total drift in seconds over a year
  

  time_t now = time(nullptr); // Get the current time
  now += secondsPerYear; // Add the calculated drift
  struct tm timeinfo;
  localtime_r(&now, &timeinfo); // Convert to local time

  char timeString[9]; // Format HH-MM-SS
  strftime(timeString, sizeof(timeString), "%H:%M:%S", &timeinfo);
  
  Serial.print("Drift in seconds per day: ");
  Serial.print(secondsPerYear);
  Serial.print(" time: ");
  Serial.println(timeString);

  display.clear();
  display.print(timeString); // Display the current time on the display

  return millis(); // Return the elapsed time since the last update
}

// Fetches data from the web service and updates the frequency gauge display
// If the timestamp has changed, updates the display with the new frequency
void fetchWebServiceData(uint8_t * payload, size_t length)
{
  String message = String((char*)payload, length);
  if (message.length() > 0) 
  {
    Serial.print("Message received via WebSocket: ");
    Serial.println(message);

    // Parse the received JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (!error) 
    {
      uint64_t newTimestamp = doc["time_stamp"];
      float frequency = doc["frequency"];

      // Integrity check for frequency
      if (frequency < minFrequency || frequency > maxFrequency) 
      {
        Serial.print("Error: frequency out of range (");
        Serial.print(frequency);
        Serial.println(" Hz)");
        return; // Ignore the received value
      }

      // Update only if the timestamp has changed
      static uint64_t lastTimestamp = 0;
      if (newTimestamp != lastTimestamp) 
      {
        lastTimestamp = newTimestamp;

        Serial.print("New Timestamp: ");
        Serial.println(lastTimestamp);
        Serial.print("New Frequency: ");
        Serial.println(frequency);
        gaugeFreqMeter.setPosition(frequency); // Update the frequency gauge with the new value

        updateDisplayWithCurrentTime(true, frequency); // Update the display with the new frequency
      } 
      else 
      {
        Serial.println("Timestamp unchanged, no update needed.");
      }
    } 
    else 
    {
      Serial.println("Error parsing JSON from WebSocket message");
    }
  }    
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type)
  {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      Serial.printf("[WSc] Connected to url: %s\n", payload);

      // Send message to server when connected
      webSocket.sendTXT("Connected");
      break;
    case WStype_TEXT:
      fetchWebServiceData(payload, length); // Fetch data from the web service and update the frequency display
        
      break;
    case WStype_BIN:
      Serial.printf("[WSc] Received binary length: %u\n", length);
      break;
  
    case WStype_ERROR:	
      Serial.println("[WSc] WebSocket error detected!");
      break;	

    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
  }
}

// --------------------- MAIN FUNCTIONS ---------------------

void setup() 
{
  
  Serial.begin(115200);
  Serial.println("Start");

  wifiManager.begin();

  // clear the NVS partition (and all preferences stored in it)
  //nvs_flash_erase(); // erase the NVS partition and...
  //nvs_flash_init(); // initialize the NVS partition.


  // Configure the timezone for Paris (UTC+1 with automatic daylight saving time adjustment)
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov", "time.google.com"); // UTC+1 offset, daylight saving enabled

  gaugeFreqMeter.begin(D4, D5, D1);

  delay(100);

  display.begin();
  display.clear();
  display.displayUnblank();

  display.print("- MDNS -"); 
  while(mdns_init()!= ESP_OK){
    delay(1000);
    Serial.println("Starting MDNS...");
  }
 
  Serial.println("MDNS started");
 
  IPAddress serverIp;
  display.print("- HOST -"); 
  while (serverIp.toString() == "0.0.0.0") 
  {
    Serial.println("Resolving host...");
    delay(250);
    serverIp = MDNS.queryHost(serverName);
  }


  display.print("-CALIB -"); // Display "START" at startup
  gaugeFreqMeter.reset(); // Reset the frequency gauge

  webSocket.begin(serverIp, websocketPort, "/"); // Start the WebSocket client
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000); // Reconnect every 5 seconds if disconnected

}

void loop() 
{
  static unsigned long lastFetch = 0;
  static unsigned long lastDisplayUpdate = 0;
  static String serialBuffer = "";

  // Lecture des caractères reçus sur la liaison série
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      // Fin de ligne reçue, on traite la commande
      serialBuffer.trim();
      if (serialBuffer.startsWith("f=")) {
        float freq = serialBuffer.substring(2).toFloat();
        Serial.print("Commande série reçue, fréquence = ");
        Serial.println(freq);
        webSocket.disconnect(); // Arrêter le service WebSocket
        gaugeFreqMeter.setPosition(freq); // Envoyer la valeur à l'aiguille
      }

      else if (serialBuffer.startsWith("p=")) {
        int pos = serialBuffer.substring(2).toInt();
        Serial.print("Commande série reçue, pos = ");
        Serial.println(pos);
        webSocket.disconnect(); // Arrêter le service WebSocket
        gaugeFreqMeter.setStep(pos); // Envoyer la valeur à l'aiguille
      }  
      serialBuffer = ""; // Réinitialiser le buffer
    } else {
      serialBuffer += c;
    }
  }

  if (millis() - lastFetch > 500) { // Fetch data every 500ms
    if(wifiManager.checkWiFiConnection())
    {
      //Serial.println("WiFi connected");
    }
    else
    {
      Serial.println("WiFi not connected, attempting reconnection...");
    }
  }

  // Update the display every second
  if (millis() - lastDisplayUpdate > 1000) 
  {
    lastDisplayUpdate = updateDisplayWithCurrentTime(false, 0.0f); // Update the display with the current time
  }
  webSocket.loop(); // Handle WebSocket events
  delay(100);
}