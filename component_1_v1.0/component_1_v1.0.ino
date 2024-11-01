
bool signupOK = false;
#include <WiFi.h>
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

// Define sensor pins
#define s1g 12  // MQ-135 for Sensor 1
#define s1f 2   // Fire sensor for Sensor 1
#define s2g 27  // MQ-135 for Sensor 2
#define s2f 26  // Fire sensor for Sensor 2
#define s3g 34  // MQ-135 for Sensor 3
#define s3f 35  // Fire sensor for Sensor 3
#define s4g 32  // MQ-135 for Sensor 4
#define s4f 33  // Fire sensor for Sensor 4

// Define output pins
#define green 23  // Green LED pin
#define red 22    // Red LED pin
#define buzzer 21 // Buzzer pin

// Calibration thresholds (will be updated after calibration)
float gasThreshold;
float fireThreshold;

// Number of samples for calibration
const int numSamples = 10;

// Initial gas readings for calibration
int gasInit[4] = {0, 0, 0, 0};

void setup() {
  Serial.begin(115200);

  // Setup sensor pins
  pinMode(s1g, INPUT);
  pinMode(s2g, INPUT);
  pinMode(s3g, INPUT);
  pinMode(s4g, INPUT);
  pinMode(s1f, INPUT);
  pinMode(s2f, INPUT);
  pinMode(s3f, INPUT);
  pinMode(s4f, INPUT);

  // Setup LED and buzzer pins
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(buzzer, OUTPUT);

 WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(red, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(red, LOW);
    delay(300);
  }
  Serial.println("\nConnected to Wi-Fi");
  digitalWrite(red, LOW);

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
  // Start-up LED sequence and calibrate sensors
  startLed();
  calibrateSensors();
}

void loop() {
  // Read sensor values
  int GsensorValue[4] = {
    analogRead(s1g),
    analogRead(s2g),
    analogRead(s3g),
    analogRead(s4g)
  };

  int FsensorValue[4] = {
    analogRead(s1f),
    analogRead(s2f),
    analogRead(s3f),
    analogRead(s4f)
  };

  // Print sensor readings
  Serial.println("Sensor Readings:");
  for (int i = 0; i < 4; i++) {
    printSensorValues(i + 1, GsensorValue[i], FsensorValue[i]);
    checkSensors(GsensorValue[i], FsensorValue[i], i);
  }

  delay(500); // Wait 500ms before the next reading
}

// Function to check both sensors for fire or gas and activate buzzer if needed
void checkSensors(int gasValue, int fireValue, int id) {
  int gasDifference = gasValue - gasInit[id];

   if ((gasValue - gasInit[id] >= 800 && gasValue - gasInit[id] < 1200 && fireValue < 250) || 
      (gasValue - gasInit[id] >= 800 && gasValue - gasInit[id] < 1200)) {
    Serial.println("Warning! Type 1 Fire and dangerous gas detected!");
    FireAlarm();
    if (Firebase.ready() && signupOK){
       if (Firebase.RTDB.setString(&fbdo, "fire_type", "Type01")){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
      if (Firebase.RTDB.setBool(&fbdo, "is_fire", true)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }   
    if (Firebase.RTDB.setBool(&fbdo, "is_fire_detected", true)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        } 
    }
         
        
  } else if ((gasValue - gasInit[id] >= 1500 && gasValue - gasInit[id] < 2500 && fireValue < 250) || 
             (gasValue - gasInit[id] >= 1500 && gasValue - gasInit[id] < 2500)) {
    Serial.println("Warning! Type 2 Fire and dangerous gas detected!");
    FireAlarm();
    if (Firebase.ready() && signupOK){
      if (Firebase.RTDB.setString(&fbdo, "fire_type", "Type02")){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
      if (Firebase.RTDB.setBool(&fbdo, "is_fire", true)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }   
    if (Firebase.RTDB.setBool(&fbdo, "is_fire_detected", true)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        } 
    }
        
  } else {
    Serial.println("No fire detected!");
     if (Firebase.ready() && signupOK){
       if (Firebase.RTDB.setBool(&fbdo, "is_fire", false)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }   
    if (Firebase.RTDB.setBool(&fbdo, "is_fire_detected", false)){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
     }
           
  }
}

// Function to print sensor values
void printSensorValues(int sensorID, int gasValue, int fireValue) {
  Serial.print("Sensor ");
  Serial.print(sensorID);
  Serial.print(" Gas: ");
  Serial.print(gasValue);
  Serial.print(" Fire: ");
  Serial.println(fireValue);
}

// Function to trigger alarm
void triggerAlarm() {
  int melody[] = {262, 294, 330, 392, 523}; // C4, D4, E4, G4, C5 notes
  int noteDurations[] = {200, 200, 200, 200, 400}; // Durations in milliseconds
  
  digitalWrite(red, HIGH);  // Turn on red LED
  digitalWrite(green, LOW); // Turn off green LED
  
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 5; j++) {
      tone(buzzer, melody[j], noteDurations[j]);
      delay(noteDurations[j] * 1.3);
    }
    delay(300);
  }
  delay(1000);
}

// Function to trigger fire alarm with a siren sound
void FireAlarm() {
  digitalWrite(red, HIGH); // Turn on red LED
  
  // Create a rising and falling siren sound
  for (int i = 0; i < 5; i++) {
    for (int frequency = 800; frequency <= 1200; frequency += 10) {
      tone(buzzer, frequency, 50);
      delay(50);
    }
    for (int frequency = 1200; frequency >= 800; frequency -= 10) {
      tone(buzzer, frequency, 50);
      delay(50);
    }
  }
  digitalWrite(red, LOW); // Turn off the red LED after the siren cycle
}

// Function to calibrate sensors
void calibrateSensors() {
  Serial.println("Calibrating sensors...");
  float gasSum[4] = {0, 0, 0, 0};

  for (int i = 0; i < numSamples; i++) {
    for (int j = 0; j < 4; j++) {
      gasSum[j] += analogRead(s1g + j * 2); // Reads s1g, s2g, s3g, s4g
    }
    delay(200); // Stabilization delay between readings
  }

  // Calculate initial readings for calibration
  for (int i = 0; i < 4; i++) {
    gasInit[i] = gasSum[i] / numSamples;
  }

  Serial.println("Calibration complete.");
  for (int i = 0; i < 4; i++) {
    Serial.print("Gas Init for Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(gasInit[i]);
  }
}

// Function for LED start-up sequence
void startLed() {
  Serial.println("LED Start Sequence");
  for (int i = 0; i < 3; i++) {
    digitalWrite(green, HIGH);
    digitalWrite(red, LOW);
    delay(500);
    digitalWrite(green, LOW);
    digitalWrite(red, HIGH);
    delay(500);
  }
  digitalWrite(green, HIGH);
  digitalWrite(red, LOW);
  delay(1000);
}
