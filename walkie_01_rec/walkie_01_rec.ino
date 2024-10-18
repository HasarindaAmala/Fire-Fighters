#include <WiFi.h>
#include <driver/i2s.h>
#include <WiFiUdp.h>

#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 8000
#define I2S_SAMPLE_BITS 16
#define I2S_READ_LEN 1024
#define BUTTON_PIN 34

const char* ssid = "Fedooora";
const char* password = "24942494";
const char* udpAddress = "172.20.10.5"; // Receiver ESP32 IP address
const int udpPort = 12345;

WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize I2S for recording
  i2sInit();
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    recordAndSendAudio();
  }
}

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
