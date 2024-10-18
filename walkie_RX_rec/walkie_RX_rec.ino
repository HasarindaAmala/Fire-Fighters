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

const char* ssid = "Fedooora";
const char* password = "24942494";
const int udpPort = 12345;

WiFiUDP udp;
char udpBuffer[I2S_READ_LEN];

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  udp.begin(udpPort);
  i2sInit();
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    size_t len = udp.read(udpBuffer, I2S_READ_LEN);
     for (int i = 0; i < len; i += 2) {
      int16_t* sample = (int16_t*)(udpBuffer + i);
      *sample *= 2; // Adjust multiplier to increase volume
    }
    i2s_write(I2S_PORT, udpBuffer, len, &len, portMAX_DELAY);
  }
}

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_SD,
    .data_in_num = -1
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}
