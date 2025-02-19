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
const char* ssid = "Fedooora";
const char* password = "24942494";

FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;

unsigned long sendDataPrevMillis = 0;


#define s1g 12  // MQ-135 for Sensor 1
#define s1f 2   // Fire sensor for Sensor 1
#define s2g 27  // MQ-135 for Sensor 2
#define s2f 26  // Fire sensor for Sensor 2
#define s3g 34  // MQ-135 for Sensor 3
#define s3f 35  // Fire sensor for Sensor 3
#define s4g 32  // MQ-135 for Sensor 4
#define s4f 33  // Fire sensor for Sensor 4
#define green 23  // Green LED pin
#define red 22    // Red LED pin
#define buzzer 21 // Buzzer pin

float gasThreshold;
float fireThreshold;
const int numSamples = 10;
int gasInit[5] = {0, 0, 0, 0, 0};

void printSensorValues(int sensorID, int gasValue, int fireValue);
void checkSensors(int gasValue, int fireValue, int id);
void FireAlarm();
void calibrateSensors();


void setup() {
  Serial.begin(115200);
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
   delay(1000);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(s1g, INPUT);
  pinMode(s2g, INPUT);
  pinMode(s3g, INPUT);
  pinMode(s4g, INPUT);
  pinMode(s1f, INPUT);
  pinMode(s2f, INPUT);
  pinMode(s3f, INPUT);
  pinMode(s4f, INPUT);

  
    
}

void loop() {
  if (Firebase.ready() && signupOK ) {
   

    int GsensorValue1 = analogRead(s1g);
    int GsensorValue2 = analogRead(s2g);
    int GsensorValue3 = analogRead(s3g);
    int GsensorValue4 = analogRead(s4g);

    int FsensorValue1 = analogRead(s1f);
    int FsensorValue2 = analogRead(s2f);
    int FsensorValue3 = analogRead(s3f);
    int FsensorValue4 = analogRead(s4f);

    Serial.println("Sensor Readings:");
    printSensorValues(1, GsensorValue1, FsensorValue1);
    printSensorValues(2, GsensorValue2, FsensorValue2);
    printSensorValues(3, GsensorValue3, FsensorValue3);
    printSensorValues(4, GsensorValue4, FsensorValue4);

    checkSensors(GsensorValue1, FsensorValue1, 1);
    checkSensors(GsensorValue2, FsensorValue2, 2);
    checkSensors(GsensorValue3, FsensorValue3, 3);
    checkSensors(GsensorValue4, FsensorValue4, 4);

    delay(500);
  }
}

void checkSensors(int gasValue, int fireValue, int id) {
  String sensorPath = "sensor/s" + String(id);
  if (gasValue > 0) {
   
    if (Firebase.RTDB.setString(&fbdo, sensorPath,"on")){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
  }if (gasValue == 0) {
    if (Firebase.RTDB.setString(&fbdo, sensorPath,"off")){
          Serial.println(F("PASSED"));
        
        } else {
          Serial.println(F("FAILED"));
         
        }
  }

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

void printSensorValues(int sensorID, int gasValue, int fireValue) {
  Serial.print("Sensor ");
  Serial.print(sensorID);
  Serial.print(": Gas Value = ");
  Serial.print(gasValue);
  Serial.print(", Fire Value = ");
  Serial.println(fireValue);
}

void triggerAlarm() {
  int melody[] = {262, 294, 330, 392, 523};
  int noteDurations[] = {200, 200, 200, 200, 400};

  digitalWrite(red, HIGH);
  digitalWrite(green, LOW);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 5; j++) {
      tone(buzzer, melody[j], noteDurations[j]);
      delay(noteDurations[j] * 1.3);
    }
    delay(300);
  }
  delay(1000);
}

void FireAlarm() {
  digitalWrite(red, HIGH);

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
  digitalWrite(red, LOW);
}

void calibrateSensors() {
  Serial.println("Calibrating sensors...");
    int GsensorValue1 = analogRead(s1g);
    int GsensorValue2 = analogRead(s2g);
    int GsensorValue3 = analogRead(s3g);
    int GsensorValue4 = analogRead(s4g);
    gasInit[1] = GsensorValue1;
    gasInit[2] = GsensorValue2;
    gasInit[3] = GsensorValue3;
    gasInit[4] = GsensorValue4;
//  float gasSum[4] = {0, 0, 0, 0};
//
//  for (int i = 0; i < numSamples; i++) {
//    for (int j = 0; j < 4; j++) {
//      gasSum[j] += analogRead(s1g + j * 2);
//    }
//    delay(200);
//  }
//
//  for (int i = 0; i < 4; i++) {
//    gasInit[i] = gasSum[i] / numSamples;
//  }
//
//  Serial.println("Calibration complete.");
//  for (int i = 0; i < 4; i++) {
//    Serial.print("Gas Init for Sensor ");
//    Serial.print(i + 1);
//    Serial.print(": ");
//    Serial.println(gasInit[i]);
//  }
}

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
  digitalWrite(green, LOW);
  digitalWrite(red, LOW);
}
