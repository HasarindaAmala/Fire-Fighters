#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050_tockn.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  // Initialize the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Initialize the IMU6050
 
   
    // Calculate offsets (make sure sensor is still)

  Serial.println("MPU6050 ready!");
}

void loop() {
  // Get IMU data
   mpu6050.update();
if(millis()-timer >500){
  // Clear the display buffer
  display.clearDisplay();
  display.setCursor(0, 0);

  // Display IMU data
  display.print("Accel X: ");
  display.println(mpu6050.getAccX());
  display.print("Accel Y: ");
  display.println(mpu6050.getAccY());
  display.print("Accel Z: ");
  display.println(mpu6050.getAccZ());

  display.display();  // Update the display

  timer = millis();
}
  
}
