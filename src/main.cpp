// ls /dev/tty*
// sudo chmod a+rw /dev/ttyUSB0
#include <iostream>
#include <string>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* ssid = "Beers R Us";
const char* password =  "dachosenjuan";
const char* my_key = "690f3c80b7f44134a5ff5b64b57a0988";  // Server URL
const char *MBTA_URL = "https://api-v3.mbta.com/";


const char* test_root_ca= \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIF1zCCBL+gAwIBAgIQBiKkIY/Slgknng9HWLomdDANBgkqhkiG9w0BAQsFADA8\n" \
  "MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRwwGgYDVQQDExNBbWF6b24g\n" \
  "UlNBIDIwNDggTTAyMB4XDTI1MDIyNTAwMDAwMFoXDTI2MDMyNjIzNTk1OVowEzER\n" \
  "MA8GA1UEAxMIbWJ0YS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\n" \
  "AQDSo1JmPthh9UngaavqodRvtshdR1NW29dDkbq7pXdQKxDNKcWsseCrn8XQHIld\n" \
  "Xl1MP7N6OqDyxxMPb2hGCCpWgVOrDJpUIkL0HIsOrl6/2j1UalFw9dJXuEUBxNE7\n" \
  "V2H6yT0YjlSL8qgGii53IR+5znVzrVRdSAVFze4TRCSer8++MNCOYt+SWITLIhez\n" \
  "HjLALlkqAu3Di/5GMetYr9nLZR9qH8vz3OeaFNhBz+NiIwGc0gJv0fwCPXEaKd4b\n" \
  "Y9mKKoeg/19UiTOsRs0C49nz0HJ+hAQ8X7uY/pGvyTvvXYsNkwrobxbAPi3IdY+F\n" \
  "BdyEXnueRLcnTxERw8kx0lNVAgMBAAGjggL8MIIC+DAfBgNVHSMEGDAWgBTAMVLN\n" \
  "WlDDgnx0cc7L6Zz5euuC4jAdBgNVHQ4EFgQUhlmLVqMR5uzttBXDlhA3IhFy0Ccw\n" \
  "LQYDVR0RBCYwJIIIbWJ0YS5jb22CCioubWJ0YS5jb22CDCoubWJ0YWNlLmNvbTAT\n" \
  "BgNVHSAEDDAKMAgGBmeBDAECATAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYI\n" \
  "KwYBBQUHAwEGCCsGAQUFBwMCMDsGA1UdHwQ0MDIwMKAuoCyGKmh0dHA6Ly9jcmwu\n" \
  "cjJtMDIuYW1hem9udHJ1c3QuY29tL3IybTAyLmNybDB1BggrBgEFBQcBAQRpMGcw\n" \
  "LQYIKwYBBQUHMAGGIWh0dHA6Ly9vY3NwLnIybTAyLmFtYXpvbnRydXN0LmNvbTA2\n" \
  "BggrBgEFBQcwAoYqaHR0cDovL2NydC5yMm0wMi5hbWF6b250cnVzdC5jb20vcjJt\n" \
  "MDIuY2VyMAwGA1UdEwEB/wQCMAAwggF/BgorBgEEAdZ5AgQCBIIBbwSCAWsBaQB3\n" \
  "AA5XlLzzrqk+MxssmQez95Dfm8I9cTIl3SGpJaxhxU4hAAABlTwj5LgAAAQDAEgw\n" \
  "RgIhAIUYNcXH43tA2RSXgaLld5jr++Fr31++6nM9xIO9E0TXAiEAtQeD8zr52//6\n" \
  "BNTx97kzC8301HYqTOmnI4Pq/tnoqpYAdgBkEcRspBLsp4kcogIuALyrTygH1B41\n" \
  "J6vq/tUDyX3N8AAAAZU8I+TvAAAEAwBHMEUCIQDDZBBD+ff6pxPeJparlGkq/ehk\n" \
  "gjCsCtJEv1ngw+t4jgIga9GOkCYCI0aTy+dxETrPgnCqN28tuUCFRzzeGWJfBr0A\n" \
  "dgBJnJtp3h187Pw23s2HZKa4W68Kh4AZ0VVS++nrKd34wwAAAZU8I+T6AAAEAwBH\n" \
  "MEUCICt1FHI8zc14MmP/PVJGdobqTyFIS4/L/yRnIQA+/wgZAiEAtD81K3oWiG3c\n" \
  "ckeFqMDSp/kCPPpGAHVi6kPf9Q2ec38wDQYJKoZIhvcNAQELBQADggEBAAiyT/ZN\n" \
  "T79SfPB2L5oQzfFOGzqq0kCBAP2ikYGGuFn5aUPB1uLZpWAPMvOzPzTCYATtG51A\n" \
  "MGjXDpVI7d9LGKKsSrWlTgjr0cDYLGMX9Fd5k1OrDiHnAshoPD4HOJZfna8vOSJT\n" \
  "IUp5XnZX/gJRpQX48WO/BPVtCu7iDQF+3sde2aW/eDxmsqMBRbfAycfi1QJkOBpJ\n" \
  "LQkmOMUNX1soLlo3vL+iQ29l/FBAAEuwpIaIHLpK7kLVnY++fY85Omrhlm2FgOJU\n" \
  "Q5wgKkAe5nAd0Mn++EXp8iLWK2wNi6/a6BdIY8k5oakbDfVNfrRdTiOjGLCf/cTM\n" \
  "XkA9BXCj+b/TSAA=\n" \
  "-----END CERTIFICATE-----\n";

// WiFiClientSecure client;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);

  // pinMode(LED, OUTPUT);
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

}

WiFiClientSecure *client = new WiFiClientSecure;
HTTPClient https;
// void get_info(String category, String filter, String filter_val);

String getInfo(String category, String filter, String filter_val) {

  String targetUrl = MBTA_URL + category + "?api_key=" + my_key + "&filter[" + filter + "]=" + filter_val;
  Serial.print(targetUrl);
  if (https.begin(*client, targetUrl))
    { // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          // print server response payload
          String payload = https.getString();
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
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
    return emptyString;
  }
}


void processResults(String apiResponse) {
  // const char* input = apiResponse;
  const char* json = apiResponse.c_str();
  JsonDocument doc;
  // deserializeJson(doc, apiResponse);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print("deserializeJson() returned ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray data = doc["data"];
  for (JsonObject vehicle : data){
    String id = vehicle["id"];
    String stop_id = vehicle["relationships"]["stop"]["data"]["id"];
    String status = vehicle["attributes"]["current_status"];

    Serial.println("ID = " + id + ", STOP_ID = " + stop_id + ", STATUS = " + status);

  }
}



void loop() {
  String apiResponse;

  unsigned long startTime = millis();

  if (client) {
    // client->setCACert(test_root_ca);
    client->setInsecure();
    apiResponse = getInfo("vehicles", "route", "Orange");
  }

  //Serial.println(apiResponse);
  processResults(apiResponse);
  unsigned long endTime = millis();
  Serial.print("Elapsed time (ms): ");
  Serial.println(endTime - startTime);
  Serial.println("Waiting 2min before the next round...");
  delay(120000);
}


