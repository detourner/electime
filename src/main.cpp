#include "WifiManager.h"
#include "HardwareSerial.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* apSSID = "MazeSliderMachine";
const char* apPassword = "12345678";

WifiManager wifiManager(apSSID, apPassword);


// --------------------- VARIABLES GLOBALES ---------------------

uint64_t lastTimestamp = 0;
float lastFrequency = 0;


// --------------------- STRUCTURES ---------------------

struct FreqPoint {
  uint64_t timestamp; // en ms
  float frequency;
};

// --------------------- BUFFER CIRCULAIRE ---------------------

const int MAX_BUFFER = 128;
FreqPoint buffer[MAX_BUFFER];
int head = 0;
int tail = 0;

bool bufferIsEmpty() {
  return head == tail;
}

bool bufferIsFull() {
  return ((tail + 1) % MAX_BUFFER) == head;
}

void bufferPush(FreqPoint pt) {
  if (!bufferIsFull()) {
    buffer[tail] = pt;
    tail = (tail + 1) % MAX_BUFFER;
  } else {
    Serial.println("⚠️ Buffer plein, données perdues !");
  }
}

bool bufferTop(FreqPoint &pt) {
  if (!bufferIsEmpty()) {
    pt = buffer[head];
    head = (head + 1) % MAX_BUFFER;
    return true;
  }
  return false;
}

bool bufferPeek(FreqPoint &pt) {
  if (!bufferIsEmpty()) {
    pt = buffer[tail];
    return true;
  }
  return false;
}

int bufferCount() 
{
  if (tail >= head) 
  {
    return tail - head;
  } 
  else 
  {
    return MAX_BUFFER - head + tail;
  }
}

// --------------------- INTERPOLATION ---------------------
uint64_t latestBufferedTimestamp = 0;  // dernier timestamp déjà dans le buffer

void interpolateAndBuffer(uint64_t t1, float f1, uint64_t t2, float f2) {
  int steps = (t2 - t1) / 1000;
  printf("Interpolation de (%llu;%f) à (%llu;%f) (%d étapes)\n", t1, f1, t2, f2, steps);
  for (int i = 1; i <= steps; i++) {
    uint64_t ts = t1 + i * 1000;

    if (ts <= latestBufferedTimestamp) {
      continue; // Ne pas ajouter de doublon
    }

    FreqPoint pt;
    pt.timestamp = ts;
    pt.frequency = f1 + ((f2 - f1) * i / steps);
    bufferPush(pt);

    // Mettre à jour le timestamp du dernier point stocké
    if (ts > latestBufferedTimestamp) {
      latestBufferedTimestamp = ts;
    }
  }
}

void setup() 
{

  // clear the NVS partition (and all preferences stored in it)
  //nvs_flash_erase(); // erase the NVS partition and...
  //nvs_flash_init(); // initialize the NVS partition.

  wifiManager.begin();
  Serial.begin(115200);
  Serial.println("Start");
}

void loop() 
{
  static unsigned long lastFetch = 0;  
  static unsigned long lastMillisPotentiometer = 0;
  static unsigned char hb = 0;
  const char* apiUrl = "https://data.swissgrid.ch/charts/frequency/?lang=fr";

  if (millis() - lastFetch > 1000)
  {
    lastFetch = millis();

    if(wifiManager.checkWiFiConnection())
    {
      Serial.println("o");
      HTTPClient http;
      http.begin(apiUrl);
      int httpCode = http.GET();

      if (httpCode == 200) 
      {
        String payload = http.getString();
        DynamicJsonDocument doc(20000);
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) 
        {
          JsonArray data = doc["data"]["series"][0]["data"].as<JsonArray>();
          uint64_t ts = data[data.size() - 1][0].as<uint64_t>();
          float freq = data[data.size() - 1][1].as<float>();

          FreqPoint ptPeek;
          if(bufferTop(ptPeek))
          {
            if(ts > ptPeek.timestamp)
            {
              printf("Nouveau point %llu / %llu: \n", ptPeek.timestamp, ts);
              interpolateAndBuffer(ptPeek.timestamp, ptPeek.frequency, ts, freq);  
            }
          }
          else // buffer empty
          {
            printf("premier point %llu: \n", ts);
            FreqPoint pt;
            pt.timestamp = ts;
            pt.frequency = freq;
            bufferPush(pt);
          }
        } 
        else 
        {
          Serial.println("Erreur JSON");
        }
      } 
      else 
      {
        Serial.printf("Erreur HTTP : %d\n", httpCode);
      }
      http.end();
    }
    else
    {
      Serial.println("-");
    }
    Serial.println(hb++);
  // put your main code here, to run repeatedly:
  }

  static unsigned long lastDisplay = 0;
    // --- 2. Affichage de la fréquence toutes les 1 sec ---
  if (millis() - lastDisplay >= 1000) {
    lastDisplay = millis();

    FreqPoint pt;
    if (bufferCount() > 10 && bufferPop(pt)) {
      Serial.printf("d: %llu -> %.3f Hz (buffer), buf=%d\n", pt.timestamp, pt.frequency, bufferCount());
    } else {
      Serial.println("⏳ En attente de nouvelles données...");
    }
  }
  delay(100);
}
