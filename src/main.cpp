#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    60
#define BRIGHTNESS  5
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
int ledIndexToBlink = 3; // for example, blink the 4th LED
QueueHandle_t ledQueue;

struct BlinkCommand {
    int indices[10];  // Up to 10 LEDs
    int count;        // How many LEDs to blink
};

// Task handle
TaskHandle_t LEDTaskHandle;

// This function runs on core 1
void LEDBlinkTask(void * parameter) {
    BlinkCommand currentCommand = { .count = 0 };
  
    while (true) {
      BlinkCommand newCommand;
  
      // Check if new data is available
      if (xQueueReceive(ledQueue, &newCommand, 0) == pdPASS) {
        currentCommand = newCommand;
      }
  
      // Clear all LEDs
      fill_solid(leds, NUM_LEDS, CRGB::Black);
  
      // Light up the selected ones
      for (int i = 0; i < currentCommand.count; i++) {
        int idx = currentCommand.indices[i];
        if (idx >= 0 && idx < NUM_LEDS) {
          leds[idx] = CRGB(255, 80, 0); // Orange-ish
        }
      }
  
      FastLED.show();
      vTaskDelay(500 / portTICK_PERIOD_MS);
  
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  }


void setup() {
    Serial.begin(115200);
    delay(1000);
    // Setup FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    ledQueue = xQueueCreate(5, sizeof(BlinkCommand));  // Queue holds 5 commands


    // Create LED task pinned to core 1
    xTaskCreatePinnedToCore(
        LEDBlinkTask,      // Function to run
        "LEDBlinkTask",    // Task name
        2048,              // Stack size
        &ledIndexToBlink,              // Parameters
        1,                 // Priority
        &LEDTaskHandle,    // Task handle
        0                  // Core ID (0 or 1)
    );
}


void loop() {
    static int phase = 0;
  
    BlinkCommand cmd;
    
    if (phase % 2 == 0) {
      cmd = { .indices = {0, 2, 4}, .count = 3 };
    } else {
      cmd = { .indices = {1, 3, 5}, .count = 3 };
    }
  
    xQueueSend(ledQueue, &cmd, 10 / portTICK_PERIOD_MS);
  
    phase++;
    delay(10000);  // Switch every 10 seconds
  }

