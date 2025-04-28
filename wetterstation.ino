#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <HTTPClient.h>
#include "LittleFS.h"
#include "time.h"
#include "DHT.h"

#define PIN 8
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DHT11_PIN 2
DHT dht11(DHT11_PIN, DHT11);

AsyncWebServer server(80);
HTTPClient http;

const char* HOST_NAME = "http://192.168.0.19";
const char* PATH_NAME = "/insert_data.php";

char ledStatus[32] = "standby";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7200;
const int daylightOffset_sec = 0;

struct Config {
  bool led;
  long interval;
  long standby;
  long sleep;
  long highTemperature;
  long measurementInProcess;
  long noWlan;
} config;

struct State {
  float temperature;
  float humidity;
  char timestamp[64];
} state;

void writeFile(fs::FS& fs, const char* path, const char* content) {
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    return;
  }

  file.print(content);
  file.close();
}

void deleteFile(fs::FS& fs, const char* path) {
  fs.remove(path);
}

String readFile(fs::FS& fs, const char* path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return "";
  }

  String content = file.readString();
  file.close();
  return content;
}

bool checkFile(fs::FS& fs, const char* path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    return false;
  }

  file.close();
  return true;
}

long convertStringToHex(const char* str) {
  return strtoul(str + 1, NULL, 16); 
}

void loadConfig() {
  if (!checkFile(LittleFS, "/config.json")) {
    const char* config = "{\"led\":true,\"interval\":7200000,\"standby\":\"#1E90FF\",\"sleep\":\"#708090\",\"highTemperature\":\"#FFA500\",\"measurementInProcess\":\"#FFD700\",\"noWlan\":\"#FF4C4C\"}";
    writeFile(LittleFS, "/config.json", config);
  }

  StaticJsonDocument<96> doc;
  String json = readFile(LittleFS, "/config.json");
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    return;
  }

  config.led = doc["led"];
  config.interval = doc["interval"];
  config.standby = convertStringToHex(doc["standby"]);
  config.sleep = convertStringToHex(doc["sleep"]);
  config.highTemperature = convertStringToHex(doc["highTemperature"]);
  config.measurementInProcess = convertStringToHex(doc["measurementInProcess"]);
  config.noWlan = convertStringToHex(doc["noWlan"]);
}

void updateConfig(JsonPair& kv) {
  StaticJsonDocument<96> doc;
  String configFile = readFile(LittleFS, "/config.json");
  DeserializationError error = deserializeJson(doc, configFile);

  if (error) {
    return;
  }

  doc[kv.key()] = kv.value();
  const char* key = kv.key().c_str();

  if (strcmp(key, "led") == 0) {
    config.led = kv.value().as<bool>();
  } else if (strcmp(key, "interval") == 0) {
    config.interval = kv.value().as<long>();
  } else if (strcmp(key, "standby") == 0) {
    config.standby = convertStringToHex(kv.value());
  } else if (strcmp(key, "sleep") == 0) {
    config.sleep = convertStringToHex(kv.value());
  } else if (strcmp(key, "highTemperature") == 0) {
    config.highTemperature = convertStringToHex(kv.value());
  } else if (strcmp(key, "measurementInProcess") == 0) {
    config.measurementInProcess = convertStringToHex(kv.value());
  } else if (strcmp(key, "noWlan") == 0) {
    config.noWlan = convertStringToHex(kv.value());
  }

  char buffer[256];
  serializeJson(doc, buffer);

  writeFile(LittleFS, "/config.json", buffer);
}

void setStatus(const char* status) {
  strcpy(ledStatus, status);
}

void updateLED() {
  if (!config.led) {
    pixels.clear();
  } else if (strcmp(ledStatus, "standby") == 0) {
    pixels.setPixelColor(0, config.standby);
  } else if (strcmp(ledStatus, "sleep") == 0) {
    pixels.setPixelColor(0, config.sleep);
  } else if (strcmp(ledStatus, "highTemperature") == 0) {
    pixels.setPixelColor(0, config.highTemperature);
  } else if (strcmp(ledStatus, "measurementInProcess") == 0) {
    pixels.setPixelColor(0, config.measurementInProcess);
  } else if (strcmp(ledStatus, "noWlan") == 0) {
    pixels.setPixelColor(0, config.noWlan);
  }

  pixels.show();
}

void setup() {
  Serial.begin(115200);
  pixels.begin();
  pixels.clear();

  // WiFiManager init
  WiFiManager wm;
  if (!wm.autoConnect("Wetterstation")) {
    setStatus("noWlan");
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  dht11.begin();

  if (!LittleFS.begin(true)) {
    return;
  }

  loadConfig();
  measure();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/config.json", "application/json");
  });

  server.on("/api/measure", HTTP_GET, [](AsyncWebServerRequest* request) {
    measure();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  server.on("/api/data/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<96> doc;
    doc["timestamp"] = state.timestamp;
    doc["temperature"] = state.temperature;

    String jsonStr;
    serializeJson(doc, jsonStr);

    request->send(200, "application/json", jsonStr);
  });

  server.on("/api/data/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<96> doc;
    doc["timestamp"] = state.timestamp;
    doc["humidity"] = state.humidity;

    String jsonStr;
    serializeJson(doc, jsonStr);

    request->send(200, "application/json", jsonStr);
  });

  server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest* request) {
    StaticJsonDocument<96> doc;
    doc["timestamp"] = state.timestamp;
    doc["humidity"] = state.humidity;
    doc["temperature"] = state.temperature;

    String jsonStr;
    serializeJson(doc, jsonStr);

    request->send(200, "application/json", jsonStr);
  });

  server.on(
    "/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      String json = "";
      for (size_t i = 0; i < len; i++) {
        json += (char)data[i];
      }

      StaticJsonDocument<96> doc;
      DeserializationError error = deserializeJson(doc, json);
      if (error) {
        return;
      }

      JsonObject obj = doc.as<JsonObject>();
      JsonPair kv = *obj.begin();

      updateConfig(kv);

      request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

  server.serveStatic("/", LittleFS, "/");

  server.begin();
}

bool measured = false;
unsigned long previousMeasureTime = 0;

void insertDataIntoDB() {
  String queryString = "?";
  queryString += "temperature=" + String(state.temperature);
  queryString += "&humidity=" + String(state.humidity);
  queryString += "&timestamp=" + String(state.timestamp);

  http.begin(String(HOST_NAME) + String(PATH_NAME) + queryString);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void measure() {
  setStatus("measurementInProcess");
  updateLED();

  float humi = dht11.readHumidity();
  float tempC = dht11.readTemperature();

  if (isnan(humi) || isnan(tempC)) {
    setStatus("standby");
    return;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    setStatus("standby");
    return;
  }

  char isoTime[32];
  strftime(isoTime, sizeof(isoTime), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

  state.humidity = humi;
  state.temperature = tempC;
  strcpy(state.timestamp, isoTime);

  insertDataIntoDB();
  measured = true;
  previousMeasureTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  updateLED();
  currentTime = millis();
  if (currentTime - previousMeasureTime >= config.interval) {
    previousMeasureTime = currentTime;
    measure();
  }

  if (currentTime - previousMeasureTime >= 3000 && measured) {
    setStatus("standby");
    measured = false;
  }
}

