#include <driver/i2s.h>


float alpha = 0.2;  // Adjust alpha for cutoff frequency (0 < alpha < 1)
int32_t prev_sample = 0;  // For high-pass filter
int32_t filtered_sample = 0;  // For storing the filtered value

#define NOISE_THRESHOLD 0

#define I2S_WS 15
#define I2S_SD 22
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0

#define SAMPLE_BUFFER_SIZE 512
//#define SAMPLE_BUFFER_SIZE 64

#define SAMPLE_RATE 44100
//#define SAMPLE_RATE 1000

// most microphones will probably default to left channel but you may need to tie the L/R pin high:
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT

// also see .bits_per_sample below:
//int16_t sBuffer[SAMPLE_BUFFER_SIZE];
int32_t sBuffer[SAMPLE_BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  Serial.println("Setup I2S ...");

  delay(1000);
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  delay(500);
}

void loop() {

  for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i) {
    sBuffer[i] = 0;
  }

  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sBuffer, sizeof(sBuffer), &bytesIn, portMAX_DELAY);
  if (result == ESP_OK)
  {
    int samples_read = bytesIn / sizeof(sBuffer[0]);
    if (samples_read > 0) {
      //Serial.println(samples_read);
      for (int i = 0; i < samples_read; ++i) {
        filtered_sample = alpha * sBuffer[i] + (1 - alpha) * filtered_sample;
         if (abs(filtered_sample) > NOISE_THRESHOLD) {
          Serial.println(filtered_sample);
        }
      }
    }
  }
  else
  {
    Serial.println("Error: i2s_read() failed");
  }
}

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    //.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,      // L/R to high - left, L/R to ground - right channel
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0, // default interrupt priority
    //.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}
