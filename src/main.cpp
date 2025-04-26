#include <FastLED.h>
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>

#define LED_PIN     5
#define NUM_LEDS    60
#define BRIGHTNESS  5
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

const int MAX_STOPS = 20;     // Max number of stops

const char* ssid = "Beers R Us";
const char* password =  "dachosenjuan";
const char* my_key = "690f3c80b7f44134a5ff5b64b57a0988";  // Server URL
const char *MBTA_URL = "https://api-v3.mbta.com/";
HTTPClient https;
WiFiClientSecure *client = new WiFiClientSecure;
TaskHandle_t LEDTaskHandle;
CRGB leds[NUM_LEDS];
QueueHandle_t ledQueue;

struct LineStops {
  String id;
  String name;
};
struct OrangeLine {
  int* solidIndices;   // Up to 10 LEDs
  int count;              // How many LEDs to blink
};
std::vector<LineStops> allStops; // dynamic array
String activeOrangeStops[MAX_STOPS];
int LEDIndices[20];
std::map<String, int> stationIndexMap = {
  {"Oak Grove", 0},
  {"Malden Center", 1},
  {"Wellington", 2},
  {"Assembly", 3},
  {"Sullivan Square", 4},
  {"Community College", 5},
  {"North Station", 6},
  {"Haymarket", 7},
  {"State", 8},
  {"Downtown Crossing", 9},
  {"Chinatown", 10},
  {"Tufts Medical Center", 11},
  {"Back Bay", 12},
  {"Massachusetts Avenue", 13},
  {"Ruggles", 14},
  {"Roxbury Crossing", 15},
  {"Jackson Square", 16},
  {"Stony Brook", 17},
  {"Green Street", 18},
  {"Forest Hills", 19}
};

void clearStops() {
  for (int i = 0; i < MAX_STOPS; i++) {
    activeOrangeStops[i] = "";  // Clear individual strings
    LEDIndices[i] = -1;
  }
}

String replaceCharWithString(String input, char toReplace, String replacement) {
  String result = "";
  for (int i = 0; i < input.length(); i++) {
    if (input[i] == toReplace) {
      result += replacement;
    } else {
      result += input[i];
    }
  }
  return result;
}

String getInfo(String category, String filter, String filter_val) {

  String targetUrl = MBTA_URL + category + "?api_key=" + my_key + "&filter[" + filter + "]=" + filter_val;
  String targetUrlEdt = replaceCharWithString(targetUrl, ' ', "%20");
  // Serial.println(targetUrl);
  Serial.println(targetUrlEdt);
  if (client) {
    // client->setCACert(test_root_ca);
    client->setInsecure();
    if (https.begin(*client, targetUrlEdt)){
      // Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        // Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          // print server response payload
          String payload = https.getString();
          // Serial.println("gets here");
          https.end();
          return payload;
          //Serial.println(payload);
        }
      }
      else
      {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        return emptyString;
      }
      https.end();
    }
    else{
      Serial.printf("[HTTPS] Unable to connect\n");
      return emptyString;
    }
  }
  return emptyString;
}

String getStopName(String stop_id) {

  String apiResponse = getInfo("stops", "id", stop_id);
  // Serial.println(apiResponse);
  // Serial.println("##############");
  // Serial.println(apiResponse);
  const char* json = apiResponse.c_str();
  JsonDocument doc;
  // deserializeJson(doc, apiResponse);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return emptyString;
  }
  return doc["data"][0]["attributes"]["name"];
}

String getStopNameFromVector(String stop_id) {
  int dashIndex = stop_id.indexOf('-');
  if (dashIndex != -1){
    return stop_id.substring(0, dashIndex);
  }
  for (LineStops& stop : allStops) {
    if (String(stop.id) == stop_id) {
      return stop.name;
    }
  }
  return "Unknown Stop";
}

void loadAllStops(String type) {
  String response = getInfo("stops", "route_type", type); // batch request
  //Serial.println(response);
  const char* json = response.c_str();
  JsonDocument doc;  // specify the size
  DeserializationError error = deserializeJson(doc, F(json));
  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return;
  }
  JsonArray data = doc["data"];

  for (JsonObject stop : data) {
    LineStops info;
    info.id = stop["id"].as<String>();
    info.name = stop["attributes"]["name"].as<String>();
    allStops.push_back(info);
    // Serial.println("added");
    // Serial.println(stop["id"].as<String>());
    // Serial.println(info.id);
    // Serial.println(ESP.getFreeHeap());
  }

  for (int i=0; i++; i<20){
    LEDIndices[i] = -1;
  }
}

void processResults(String apiResponse) {
  int i = 0;
  // const char* input = apiResponse;
  const char* json = apiResponse.c_str();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return;
  }

  clearStops();
  JsonArray data = doc["data"];
  
  Serial.println("Active Orange Line Stops: ");
  for (JsonObject vehicle : data){
    const char* id = vehicle["id"];
    const char* stop_id = vehicle["relationships"]["stop"]["data"]["id"];
    const char* status = vehicle["attributes"]["current_status"];
    int direction = vehicle["attributes"]["direction_id"];

    String stopName = getStopNameFromVector(String((const char*)stop_id));//(const char*)stopIDAsjusted);
    Serial.println(stopName);
    Serial.print("ID = ");
    Serial.print(stop_id);
    activeOrangeStops[i] = stopName;



    Serial.println();
    i++;
  }
}


// This function runs on core 1
void LEDBlinkTask(void * parameter) {
    OrangeLine currentCommand = { .count = 0 };
  
    while (true) {
        OrangeLine newCommand;
  
      // Check if new data is available
      if (xQueueReceive(ledQueue, &newCommand, 0) == pdPASS) {
        currentCommand = newCommand;
      }
  
      // Clear all LEDs
      // fill_solid(leds, NUM_LEDS, CRGB::Black);
  
      // Light up the selected ones
      // Create a temporary mask
      bool keepLit[NUM_LEDS] = { false };

      // Mark LEDs that should stay on
      for (int i = 0; i < currentCommand.count; i++) {
        int idx = currentCommand.solidIndices[i];
        if (idx >= 0 && idx < NUM_LEDS) {
          keepLit[idx] = true;
          leds[idx] = CRGB(255, 80, 0); // Orange-ish
        }
      }

      // Turn off the rest
      for (int i = 0; i < NUM_LEDS; i++) {
        if (!keepLit[i]) {
          leds[i] = CRGB::Black;
        }
      }
  
      FastLED.show();
      vTaskDelay(500 / portTICK_PERIOD_MS);
  
      // fill_solid(leds, NUM_LEDS, CRGB::Black);
      // FastLED.show();
      // vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }

void getLEDs(){
  int index;
  int i = 0;
  for (String stop : activeOrangeStops){
    if (stop.isEmpty() || stop == "" || stop == "Unknown Stop"){
      LEDIndices[i] = -1;
    }
    else{
      index = stationIndexMap[stop];
      LEDIndices[i] = index;
    }
    i++;
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  } 
  Serial.println("Connected to the WiFi network");

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  allStops.reserve(110);

  loadAllStops("1");
  // loadAllStops("0");
  Serial.println("Stops Found");
  for (LineStops& stop : allStops) {
    Serial.print("Stop ID: ");
    Serial.print(stop.id);
    Serial.print(" | Stop Name: ");
    Serial.print(stop.name);
    Serial.println();
  }
  Serial.println();

  // Setup FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  ledQueue = xQueueCreate(5, sizeof(OrangeLine));  // Queue holds 5 commands
  // Create LED task pinned to core 1
  xTaskCreatePinnedToCore(
      LEDBlinkTask,      // Function to run
      "LEDBlinkTask",    // Task name
      2048,              // Stack size
      NULL,              // Parameters
      1,                 // Priority
      &LEDTaskHandle,    // Task handle
      0                  // Core ID (0 or 1)
  );
}


void loop() {
    unsigned long startTime = millis();
    // static int phase = 0;
    OrangeLine currentOrangeLine;
    String apiResponse;

    // if (phase % 2 == 0) {
    //     currentOrangeLine = { .solidIndices = {1, 5}, .count = 3 };
    // } else {
    //     currentOrangeLine = { .solidIndices = {2, 1}, .count = 3 };
    // }
    apiResponse = getInfo("vehicles", "route", "Orange");
    // Serial.print(apiResponse);
    processResults(apiResponse);
    Serial.println("Active OrangeLine");
    for (String stop : activeOrangeStops){
      Serial.println(stop);
    }
    getLEDs();
    for (int index : LEDIndices){
      Serial.println(index);
    }
    // getLEDs();
    // int test[] = {0,1,2,3};
    currentOrangeLine = { .solidIndices = LEDIndices, .count = 20 };

    // Serial.println(LEDIndices);

    xQueueSend(ledQueue, &currentOrangeLine, 10 / portTICK_PERIOD_MS);
    // phase++;

    //Serial.println(apiResponse);
    unsigned long endTime = millis();
    Serial.print("Elapsed time (ms): ");
    Serial.println(endTime - startTime);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("Waiting 10secs before the next round...\n");

    delay(10000);  // Switch every 10 seconds
  }

