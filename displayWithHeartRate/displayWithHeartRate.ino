TaskHandle_t Task1;
TaskHandle_t Task2;

#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



// Define display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;




const byte RATE_SIZE = 10; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0;
long irValue = 0;
float beatsPerMinute;
int beatAvg;


void setup() {
  Serial.begin(115200); 
  Wire.begin(21, 22); // SDA, SCL
  
  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 was not found. Please check wiring/power."));
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  Serial.println(F("Place your index finger on the sensor with steady pressure."));

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.setPulseAmplitudeGreen(0);

  // Create tasks
  xTaskCreatePinnedToCore(Task1code, "Task1", 4096, NULL, 1, &Task1, 0);          
  delay(500); 
  xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 1, &Task2, 1);          
  delay(500); 
}

void Task1code(void * pvParameters) {
  Serial.print(F("Task1 running on core "));
  Serial.println(xPortGetCoreID());

  for(;;) {
    irValue = particleSensor.getIR();

    if (irValue == 0) {
      Serial.println("No finger detected.");
      delay(100);
      continue;
    }

    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();

      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;

        // Calculate the average of the last RATE_SIZE readings
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
      }
      Serial.print("BPM: ");
      Serial.print(beatsPerMinute);
      Serial.print(" | Avg BPM: ");
      Serial.println(beatAvg);
    }
  }
}

void Task2code(void * pvParameters) {
  Serial.print(F("Task2 running on core "));
  Serial.println(xPortGetCoreID());

  for(;;){
    delay(1000);
    display.clearDisplay();
    display.setCursor(0, 0);

    // Display heart rate
    display.print("Heart Rate: ");
    display.print(beatAvg);
    display.println(" bpm");
    display.display(); // Update the display
   

  }
}

void loop() {}
