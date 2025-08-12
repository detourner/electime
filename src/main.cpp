#include "WifiManager.h"
#include "HardwareSerial.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "HCMS39xx.h"


const char* apSSID = "ElecTime";
const char* apPassword = "12345678";
const char* webServiceUrl = "http://example.com/api/data";

WifiManager wifiManager(apSSID, apPassword);


// --------------------- VARIABLES GLOBALES ---------------------

uint64_t lastTimestamp = 0;
float lastFrequency = 0;

// See https://github.com/Andy4495/HCMS39xx/blob/main/README.md#hardware-connections for wiring info
// HCMS39xx(uint8_t num_chars, uint8_t data_pin, uint8_t rs_pin, uint8_t clk_pin, 
//          uint8_t ce_pin, uint8_t blank_pin)
HCMS39xx display(8, 6, 7, 8, 9, 10); // osc_select_pin tied high, not connected to microcontroller

void setup() 
{

  // clear the NVS partition (and all preferences stored in it)
  //nvs_flash_erase(); // erase the NVS partition and...
  //nvs_flash_init(); // initialize the NVS partition.

  wifiManager.begin();
  Serial.begin(115200);
  Serial.println("Start");

  display.begin();
  display.clear();
  display.print("START"); // Affiche "START" au démarrage
}

void fetchWebServiceData() 
{
  if (wifiManager.checkWiFiConnection()) 
  {
    HTTPClient http;
    http.begin(webServiceUrl); // Initialise la connexion HTTP
    int httpCode = http.GET(); // Effectue une requête GET

    if (httpCode > 0) 
    { // Vérifie si la requête a réussi
      if (httpCode == HTTP_CODE_OK) 
      {
        String payload = http.getString(); // Récupère la réponse JSON
        Serial.println("Réponse du serveur :");
        Serial.println(payload);

        // Parse le JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) 
        {
          lastTimestamp = doc["time_stamp"];
          lastFrequency = doc["frequency"];
          Serial.print("Timestamp : ");
          Serial.println(lastTimestamp);
          Serial.print("Frequency : ");
          Serial.println(lastFrequency);
        } 
        else 
        {
          Serial.println("Erreur de parsing JSON");
        }
      }
    } 
    else 
    {
      Serial.print("Erreur HTTP : ");
      Serial.println(httpCode);
    }
    http.end(); // Ferme la connexion HTTP
  }
   else
   {
    Serial.println("Pas de connexion WiFi");
  }
}

void loop() 
{
  static unsigned long lastFetch = 0;

  if (millis() - lastFetch > 500) { // Récupère les données toutes les 500ms
    lastFetch = millis();
    fetchWebServiceData();
  }
  delay(100);
}
