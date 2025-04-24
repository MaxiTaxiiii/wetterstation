#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"

AsyncWebServer server(80);

void writeFile(fs::FS& fs, const char* path, const char* content) {
  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open path for writing");
  }

  if(file.print(content)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

bool checkFile(fs::FS& fs, const char* path) {
  File file = fs.open(path);
  if(!file || file.isDirectory()) {
    Serial.println("Failed to open file");
    return false;
  }

  file.close();
  return true;
}

void setup() {
  Serial.begin(115200);

  // WiFiManager init
  WiFiManager wm;

  bool res;
  res = wm.autoConnect("Wetterstation");
  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("Successfully connected");
  }

  // LittleFS init
  if(!LittleFS.begin(true)) {
    Serial.println("LittleFS failed to mount");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  if(!checkFile(LittleFS,"/config.json")) {
    Serial.println("Failed to open config: generating default config");
    const char* config = "{\"ledOn\":true,\"interval\":7200000,\"statusColors\":{\"standby\":\"#1E90FF\",\"sleep\":\"#708090\",\"highTemperature\":\"#FFA500\",\"measurementInProcess\":\"#FFD700\",\"noWlan\":\"#FF4C4C\"}}";
    writeFile(LittleFS,"/config.json",config);
  }

  server.on("/",HTTP_GET,[](AsyncWebServerRequest* request){
    request->send(LittleFS,"/index.html","text/html");
  });

  server.on("/api/config",HTTP_GET,[](AsyncWebServerRequest* request) {
    request->send(LittleFS,"/config.json","application/json");
  });

  server.on("/api/config", HTTP_POST,[](AsyncWebServerRequest* request) {}, NULL,
    [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      String json = "";
      for(size_t i = 0 ; i < len ; i++) {
        json += (char)data[i];
      }
      Serial.println("Received POST request");
      Serial.println(json);

      writeFile(LittleFS,"/config.json",json.c_str());
      request->send(200,"application/json","{\"status\":\"ok\"}");
    }
  );

  server.serveStatic("/", LittleFS, "/");

  server.begin();
}

void loop() {
}
