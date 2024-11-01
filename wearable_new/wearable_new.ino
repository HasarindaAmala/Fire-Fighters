TaskHandle_t WalkieTalkie;
TaskHandle_t IMU;
TaskHandle_t Display;
TaskHandle_t HeartRate;
bool air = false;
bool signupOK = false;
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Firebase configuration
#define API_KEY "AIzaSyDa-KKJMO8BH9A025HsYBxkHhkp1phrS8E"
#define DATABASE_URL "https://firefighter-3278c-default-rtdb.firebaseio.com/"

// Wi-Fi configuration
const char* ssid = "test";
const char* password = "A12345678";

FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;

#include <WiFi.h>
#include <driver/i2s.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050_tockn.h>
#include "spo2_algorithm.h"

#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14

#define I2S_WS_S 12    // New WS (Word Select) pin
#define I2S_SD_S 33    // New SD (Serial Data) pin
#define I2S_SCK_S 13   // New SCK (Serial Clock) pin

#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 8000
#define I2S_SAMPLE_RATE_S 7000
#define I2S_SAMPLE_BITS 16
#define I2S_READ_LEN 1024
#define BUTTON_PIN 34
#define s1g 23  //MQ-135 for Sensor 1


const char* udpAddress = "172.20.10.5"; // Receiver ESP32 IP address
const int udpPort = 12345;
//const int udpPort_in = 23456;

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
int beatAvg = 65;
bool active;
float accX = 0.0;
float accY = 0.0;
float accZ = 0.0;
float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;
float accelerationMagnitude = 0.0;
float gyroMagnitude = 0.0;
float temperature = 0.0;

int GsensorValue1 = 0;
int heartRateVal = 0;
bool speaking = false;
bool listning = true;
WiFiUDP udp;
char udpBuffer[I2S_READ_LEN];
bool audioPlayed = false;  // Flag to track if the audio has been played
MPU6050 mpu6050(Wire);
long timer = 0;
size_t bytesWritten;


void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}
void i2sInit_mic() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE_S,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_S,
    .ws_io_num = I2S_WS_S,
    .data_out_num = I2S_SD_S,
    .data_in_num = -1
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

void recordAndSendAudio() {
  Serial.print("Start Recording");
  char i2s_read_buff[I2S_READ_LEN];
  size_t bytesRead;
  float volumeMultiplier = 2.0;

  while (digitalRead(BUTTON_PIN) == LOW) {
    i2s_read(I2S_PORT, (void*)i2s_read_buff, I2S_READ_LEN, &bytesRead, portMAX_DELAY);
     for (int i = 0; i < bytesRead; i += 2) {
      int16_t* sample = (int16_t*)(i2s_read_buff + i);
      *sample = (int16_t)(*sample * volumeMultiplier);

      // Ensure the sample stays within the 16-bit range to avoid clipping
      if (*sample > 32767) *sample = 32767;
      else if (*sample < -32768) *sample = -32768;
    }
  
    udp.beginPacket(udpAddress, udpPort);
    udp.write((uint8_t*)i2s_read_buff, bytesRead);
    udp.endPacket();
  }
}


void setup() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // put your setup code here, to run once:
  Serial.begin(115200); 
  Wire.begin(21, 22); // SDA, SCL
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Connecting to WiFi...");
    display.display(); 

  }
  Serial.println("Connected to WiFi");
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Connected!");
  display.display(); 
  udp.begin(udpPort);
  // Initialize I2S for recording
  i2sInit();
  //i2sInit_mic();
  
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready() && signupOK){
      
      if (Firebase.RTDB.setBool(&fbdo, "is_fighter_ard", true)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }   
    
    }
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(s1g, INPUT);
  //pinMode(s1g, INPUT_PULLDOWN);
  // Initialize the display
  
  
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 was not found. Please check wiring/power."));
    while (1);
  }

  
  Serial.println(F("Place your index finger on the sensor with steady pressure."));

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.setPulseAmplitudeGreen(0);
  particleSensor.enableDIETEMPRDY();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  // Create tasks
  xTaskCreatePinnedToCore(walkieTalkie, "TaskWalkieTalkie", 15000, NULL, 1, &WalkieTalkie, 0);          
  delay(500); 
  xTaskCreatePinnedToCore(imu, "TaskImu", 4096, NULL, 1, &IMU, 0);          
  delay(500); 
  xTaskCreatePinnedToCore(heartRate, "TaskHeartRate", 4096, NULL, 1, &HeartRate, 1);          
  delay(500); 
  xTaskCreatePinnedToCore(display_wifi, "Taskdisplay", 10000, NULL, 1, &Display, 1);          
  delay(500); 

}
void walkieTalkie(void *pvParameters){
  for(;;){
    
    if (digitalRead(BUTTON_PIN) == LOW) {
    recordAndSendAudio();
    listning = false;
    speaking =true;
  }else{
     listning = true;
    speaking =false;
//    int packetSize = udp.parsePacket();
//    if (packetSize) {
//    size_t len = udp.read(udpBuffer, I2S_READ_LEN);
//    
//    // Amplify the audio samples
//    for (size_t i = 0; i < len; i += 2) {
//      int16_t* sample = (int16_t*)(udpBuffer + i);
//      *sample *= 6;  // Adjust multiplier to increase volume
//    }
//    
//    // Check if we have new audio data and play it
//    if (!audioPlayed) {
//     
//      i2s_write(I2S_PORT, udpBuffer, len, &bytesWritten, portMAX_DELAY);
//      audioPlayed = true;  // Set the flag after playing the audio
//    } else if (bytesWritten > 0) {
//      // If audio has been played, we can reset the flag if the buffer is empty
//      audioPlayed = false;
//    }
//  } else {
//    // Reset the flag if no new packet is received
//    audioPlayed = false;  
//  }
  }
  vTaskDelay(10);
  }
}
void imu(void *pvParameters){
 for(;;){
   mpu6050.update();
   accX = mpu6050.getAccX();
   accY = mpu6050.getAccY();
   accZ = mpu6050.getAccZ();
   gyroX = mpu6050.getGyroX();
   gyroY = mpu6050.getGyroY();
   gyroZ = mpu6050.getGyroZ();
   GsensorValue1 = analogRead(s1g);
   
   accelerationMagnitude = sqrt(pow(accX, 2) + pow(accY, 2) + pow(accZ, 2));
   gyroMagnitude = sqrt(pow(gyroX, 2) + pow(gyroY, 2) + pow(gyroZ, 2));
   Serial.println("magnitude:");
   Serial.println(accelerationMagnitude);
   Serial.println("magnitude_gyro:");
   Serial.println(gyroMagnitude);
   Serial.println("gas:");
   Serial.println(GsensorValue1);
  // Detecting activity
  if (gyroMagnitude > 30.0) {
    Serial.println("Person is active");
    active = true;
  }else if(gyroMagnitude < 30){
    active = false;
  }
if(GsensorValue1>2500){
  air = false;
}
 
  delay(500);
  vTaskDelay(10);
 }
  
}

void heartRate(void *pvParameters){
  
  for(;;) {
    long irValue = particleSensor.getIR();
    temperature = particleSensor.readTemperature();
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
      heartRateVal = beatAvg;
      Serial.print("IR=");
      Serial.print(irValue);
      Serial.print("BPM: ");
      Serial.print(beatsPerMinute);
      Serial.print(" | Avg BPM: ");
      Serial.println(beatAvg);
    }
     
  }
 
}
void display_wifi(void *pvParameters){

  for(;;){
    if(speaking == true){
    display.clearDisplay();
  display.setCursor(0, 0);

  // Display IMU data
  display.print("Speaking.... ");
  display.display(); 
  }else{
     if(Firebase.ready() && signupOK && millis()-timer >500){
      timer = millis();
  // Clear the display buffer
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("   Fire Fighters");
  display.println("");
  // Display IMU data
  display.print("Heart Rate: ");
  display.println(beatAvg);
  display.print("Temperature: ");
  display.println(temperature);
  display.print("Air: ");
  if(GsensorValue1>3000){
    air = false;
    display.println("Dangerous");
  }else if(GsensorValue1<3000){
    air = true;
    display.println("Normal");
  }
   display.print("Active: ");
  if(active==true){
    display.println("yes");
  }else if(active == false){
    display.println("Check ASAP!");
  }
  display.display();  // Update the display
  if (Firebase.RTDB.setBool(&fbdo, "body_data/air_val", air) && Firebase.RTDB.setInt(&fbdo, "body_data/heart_rate", beatAvg)&& Firebase.RTDB.setBool(&fbdo, "body_data/is_active", active)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
  
}
  }
   vTaskDelay(10);
  }

  
 
}

void loop() {
  // put your main code here, to run repeatedly:

}
