#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define lF 4  // Left Forward
#define lR 3  // Left Reverse
#define RF 2  // Right Forward
#define RR 5  // Right Reverse

const uint64_t pipeIn = 0xE9E8F0F0E1LL;  // Same address as in the transmitter
RF24 radio(10, 9);  // CE, CSN pins

struct Signal {
  int spray;
  int servo;
  int gas;   // Adjusted to int to allow mapping range -255 to +255
  int turn;  // Adjusted to int to allow mapping range -255 to +255
};

Signal data;

void setup() {
  Serial.begin(9600);
  if (!radio.begin()) {
    Serial.println("Radio initialization failed!");
    while (1);
  }
  radio.openReadingPipe(1, pipeIn);
  radio.setAutoAck(false);
  radio.setCRCLength(RF24_CRC_8);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(108);
  radio.startListening();
  Serial.println("Receiver is ready and listening...");
  
  pinMode(lF, OUTPUT);
  pinMode(lR, OUTPUT);
  pinMode(RF, OUTPUT);
  pinMode(RR, OUTPUT);
  pinMode(6, OUTPUT);
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(Signal));

    // Print the received values to the Serial Monitor
    Serial.print("Spray: ");
    Serial.print(data.spray);
    Serial.print(" | Servo: ");
    Serial.print(data.servo);
    Serial.print(" | Gas: ");
    Serial.print(data.gas);
    Serial.print(" | Turn: ");
    Serial.println(data.turn);

    // Motor Control Logic
    if (data.gas > 0) { // Move forward
      digitalWrite(lF, HIGH);
      digitalWrite(RF, HIGH);
      digitalWrite(lR, LOW);
      digitalWrite(RR, LOW);
    } else if (data.gas < 0) { // Move backward
      digitalWrite(lF, LOW);
      digitalWrite(RF, LOW);
      digitalWrite(lR, HIGH);
      digitalWrite(RR, HIGH);
    } else { // Stop motors if gas is zero
      digitalWrite(lF, LOW);
      digitalWrite(RF, LOW);
      digitalWrite(lR, LOW);
      digitalWrite(RR, LOW);
    }

    // Turn control
    if (data.turn < 0) { // Turn left
      digitalWrite(lF, LOW);   // Stop left forward
      digitalWrite(lR, HIGH);  // Reverse left motor
      digitalWrite(RF, HIGH);  // Move right motor forward
      digitalWrite(RR, LOW);   // Stop right reverse
    } else if (data.turn > 0) { // Turn right
      digitalWrite(lF, HIGH);  // Move left motor forward
      digitalWrite(lR, LOW);   // Stop left reverse
      digitalWrite(RF, LOW);   // Stop right forward
      digitalWrite(RR, HIGH);  // Reverse right motor
    }
     if(data.spray >10){
      digitalWrite(6,HIGH);
     }
     if(data.spray <10){
      digitalWrite(6,LOW);
     }
    // Fine-tune the turning logic to prevent conflicts when moving forward or backward
    if (data.gas != 0 && data.turn == 0) {
      // Allow straight movement without turning adjustments
      digitalWrite(lF, data.gas > 0 ? HIGH : LOW);
      digitalWrite(RF, data.gas > 0 ? HIGH : LOW);
      digitalWrite(lR, data.gas < 0 ? HIGH : LOW);
      digitalWrite(RR, data.gas < 0 ? HIGH : LOW);
    }
    
  }
}
