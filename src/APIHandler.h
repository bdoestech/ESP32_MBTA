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

const int MAX_STOPS = 20;     // Max number of stops

const char* ssid = "Beers R Us";
const char* password =  "dachosenjuan";
const char* my_key = "690f3c80b7f44134a5ff5b64b57a0988";  // Server URL
const char *MBTA_URL = "https://api-v3.mbta.com/";
HTTPClient https;
WiFiClientSecure *client = new WiFiClientSecure;
TaskHandle_t LEDTaskHandle;
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

String replaceCharWithString(String input, char toReplace, String replacement);

String getInfo(String category, String filter, String filter_val);

String getStopName(String stop_id);

String getStopNameFromVector(String stop_id);

void loadAllStops(String type);

void processResults(String apiResponse);
