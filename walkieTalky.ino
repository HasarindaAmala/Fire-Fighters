#include <driver/i2s.h>
#include <math.h>

#define SAMPLE_BUFFER_SIZE 2048 // Buffer size for recording and playback

// Microphone settings
#define I2S_MIC_PORT I2S_NUM_0
#define I2S_MIC_WS 15
#define I2S_MIC_SD 22
#define I2S_MIC_SCK 2
#define MIC_SAMPLE_RATE 44100

// Speaker settings
#define I2S_SPK_PORT I2S_NUM_1
#define I2S_SPK_BCK_PIN 26
#define I2S_SPK_WS_PIN 25
#define I2S_SPK_DATA_PIN 27
#define SPK_SAMPLE_RATE 44100

#define BUTTON_PIN 34

// Buffers for recording and playback
int32_t recordedSamples[SAMPLE_BUFFER_SIZE];
int sampleIndex = 0;
bool isRecording = false;  // Flag to control recording state

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Setup I2S for the microphone (recording)
  i2s_install_microphone();
  i2s_start(I2S_MIC_PORT);
  
  // Setup I2S for the speaker (playback)
  i2s_install_speaker();
  i2s_start(I2S_SPK_PORT);
}

void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Button pressed - record audio
    if (!isRecording) {
      isRecording = true;
      sampleIndex = 0; // Reset sample index for new recording
      Serial.println("Recording...");
    }

    record_audio();
  } else if (isRecording) {
    // Button released - play back recorded audio
    isRecording = false;
    Serial.println("Playing back...");
    play_audio();
  }
}

// Function to record audio from the microphone
void record_audio() {
  if (sampleIndex < SAMPLE_BUFFER_SIZE) {
    size_t bytesIn = 0;
    int32_t micBuffer[SAMPLE_BUFFER_SIZE / 2];  // Temporary buffer for mic data
    
    // Read audio from I2S microphone
    esp_err_t result = i2s_read(I2S_MIC_PORT, &micBuffer, sizeof(micBuffer), &bytesIn, portMAX_DELAY);
    if (result == ESP_OK) {
      int samples_read = bytesIn / sizeof(micBuffer[0]);
      
      // Store the recorded samples in the global buffer
      for (int i = 0; i < samples_read && sampleIndex < SAMPLE_BUFFER_SIZE; i++) {
        recordedSamples[sampleIndex++] = micBuffer[i];
      }
    }
  }
}

// Function to play the recorded audio through the speaker
void play_audio() {
  for (int i = 0; i < sampleIndex; i++) {
    int16_t sample = (int16_t)(recordedSamples[i] >> 16);  // Convert 32-bit sample to 16-bit
    
    // Write to I2S speaker
    size_t bytes_written;
    i2s_write(I2S_SPK_PORT, &sample, sizeof(sample), &bytes_written, portMAX_DELAY);
  }
}

// I2S configuration for microphone (input)
void i2s_install_microphone() {
  const i2s_config_t i2s_config_mic = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = MIC_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .intr_alloc_flags = 0,
    .use_apll = false
  };

  const i2s_pin_config_t pin_config_mic = {
    .bck_io_num = I2S_MIC_SCK,
    .ws_io_num = I2S_MIC_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SD
  };

  i2s_driver_install(I2S_MIC_PORT, &i2s_config_mic, 0, NULL);
  i2s_set_pin(I2S_MIC_PORT, &pin_config_mic);
}

// I2S configuration for speaker (output)
void i2s_install_speaker() {
  const i2s_config_t i2s_config_spk = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SPK_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .intr_alloc_flags = 0,
    .use_apll = false
  };

  const i2s_pin_config_t pin_config_spk = {
    .bck_io_num = I2S_SPK_BCK_PIN,
    .ws_io_num = I2S_SPK_WS_PIN,
    .data_out_num = I2S_SPK_DATA_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_SPK_PORT, &i2s_config_spk, 0, NULL);
  i2s_set_pin(I2S_SPK_PORT, &pin_config_spk);
}
