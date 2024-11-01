#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define autoPin 3
const uint64_t pipeOut = 0xE9E8F0F0E1LL;
RF24 radio(9, 8);

struct Signal {
  int spray;
  int servo;
  int gas;
  int turn;
};

Signal data;

// For smoothing joystick values
const int numReadings = 5;
int gasReadings[numReadings];
int turnReadings[numReadings];
int servoReadings[numReadings];
int sprayReadings[numReadings];
int gasIndex = 0;
int servoIndex = 0;
int turnIndex = 0;
int sprayIndex = 0;
int totalGas = 0;
int totalTurn = 0;
int totalServo = 0;
int totalSpray = 0;

void ResetData() {
  data.spray = 0;
  data.servo = 0;
  data.gas = 0;
  data.turn = 0;
  
  for (int i = 0; i < numReadings; i++) {
    gasReadings[i] = 0;
    turnReadings[i] = 0;
    servoReadings[i] = 0;
    sprayReadings[i] = 0;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(autoPin, INPUT);
  if (!radio.begin()) {
    Serial.println("Radio initialization failed!");
    while (1);
  }
  radio.stopListening();
  radio.openWritingPipe(pipeOut);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_8);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(108);
  ResetData();
  delay(1000);
}

int mapJoystickValues(int val, int lower, int middle, int upper) {
  val = constrain(val, lower, upper);
  if (val < middle - 10) {
    // Map values from lower to middle-10 to -255 to 0
    return map(val, lower, middle - 10, -255, 0);
  } else if (val > middle + 10) {
    // Map values from middle+10 to upper to 0 to 255
    return map(val, middle + 10, upper, 0, 255);
  } else {
    // In the range near the middle, set to 0
    return 0;
  }
}

int smooth(int *readings, int &index, int &total, int numReadings, int newValue) {
  total = total - readings[index];
  readings[index] = newValue;
  total = total + readings[index];
  index = (index + 1) % numReadings;
  return total / numReadings;
}

void loop() {
  // Read and map joystick values
  int rawGas = mapJoystickValues(analogRead(A6), 12, 448, 1020);
  int rawTurn = mapJoystickValues(analogRead(A1), 12, 526, 1020);
  int spray_raw = mapJoystickValues(analogRead(A7), 12, 448, 1020);
  int rawServo = mapJoystickValues(analogRead(A3), 12, 512, 1020);
  
  // Smooth the values
  data.gas = smooth(gasReadings, gasIndex, totalGas, numReadings, rawGas);
  data.turn = smooth(turnReadings, turnIndex, totalTurn, numReadings, rawTurn);
  data.servo = smooth(servoReadings, servoIndex, totalServo, numReadings, rawServo);
  data.spray = smooth(sprayReadings, sprayIndex, totalSpray, numReadings, spray_raw);

  // Print the mapped values to the Serial Monitor
  Serial.print("Spray: ");
  Serial.print(data.spray);
  Serial.print(" | Servo: ");
  Serial.print(data.servo);
  Serial.print(" | Gas: ");
  Serial.print(data.gas);
  Serial.print(" | Turn: ");
  Serial.println(data.turn);

  // Send the data using nRF24L01
  if (!radio.write(&data, sizeof(Signal))) {
    Serial.println("Transmission failed!");
  } else {
    Serial.println("Data sent successfully.");
  }

  delay(100);  // Add a short delay to prevent overwhelming the Serial Monitor
}
