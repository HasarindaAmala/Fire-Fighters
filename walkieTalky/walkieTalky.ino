#include <driver/i2s.h>
#include <math.h>


float alpha = 0;  // Adjust alpha for cutoff frequency (0 < alpha < 1)
int32_t prev_sample = 0;  // For high-pass filter
int32_t filtered_sample = 0;  // For storing the filtered value


#define NOISE_THRESHOLD 0
#define SAMPLE_BUFFER_SIZE 4096 // Buffer size for recording and playback

// Microphone settings
#define I2S_MIC_PORT I2S_NUM_0
#define I2S_MIC_WS 15
#define I2S_MIC_SD 22
#define I2S_MIC_SCK 2
#define MIC_SAMPLE_RATE 44100
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT

// Speaker settings
#define I2S_SPK_PORT I2S_NUM_1
#define I2S_SPK_BCK_PIN 26
#define I2S_SPK_WS_PIN 25
#define I2S_SPK_DATA_PIN 27
#define SPK_SAMPLE_RATE 22050

#define BUTTON_PIN 34

// Buffers for recording and playback
int32_t sBuffer[SAMPLE_BUFFER_SIZE];
int32_t recordedSamples[SAMPLE_BUFFER_SIZE];
int sampleIndex = 0;
bool isRecording = false;  // Flag to control recording state

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Setup I2S for the microphone (recording)
  delay(1000);
  i2s_install_microphone();
  i2s_start(I2S_MIC_PORT);
  delay(500);
  
  // Setup I2S for the speaker (playback)
  delay(1000);
  i2s_install_speaker();
  i2s_start(I2S_SPK_PORT);
  delay(500);
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
    Serial.print("sample count");
    Serial.println(sampleIndex);
//    for(int i = 0; i<sampleIndex;i++){
//      Serial.print("recorded samples: ");
//       Serial.print(i+1);
//       Serial.print(") ");
//      Serial.println(recordedSamples[i]);
//    }
   play_audio();
  }
}

// Function to record audio from the microphone
void record_audio() {
   Serial.println("record audio start");
   
  for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i) {
    sBuffer[i] = 0;
  }
 
  
    size_t bytesIn = 0;

    // Read audio from I2S microphone
    esp_err_t result = i2s_read(I2S_MIC_PORT, &sBuffer, sizeof(sBuffer), &bytesIn, portMAX_DELAY);
    if (result == ESP_OK) {
      int samples_read = bytesIn / sizeof(sBuffer[0]);
      if (samples_read > 0) {
        for (int i = 0; i < samples_read; ++i) {
          filtered_sample = alpha * sBuffer[i] + (1 - alpha) * filtered_sample;
          if (abs(filtered_sample) > NOISE_THRESHOLD) {
            Serial.print(i+1);
            Serial.print(") ");
            Serial.println((int16_t)(filtered_sample >> 16));
            sampleIndex++;
          }

          // Store the sample if there is space in the recordedSamples array
          if (sampleIndex < SAMPLE_BUFFER_SIZE) {
            recordedSamples[sampleIndex] = filtered_sample; // Store the filtered sample
          }else{
            Serial.println("Buffer size exceed");
          }
        }
      }
      // Store the recorded samples in the global buffer
      
    }
    else {
      Serial.println("Error: i2s_read() failed");
    }
  
}

// Function to play the recorded audio through the speaker
void play_audio() {
  for (int i = 0; i < sampleIndex; i++) {
    int16_t sample = (int16_t)((recordedSamples[i]*100) >> 16);  // Convert 32-bit sample to 16-bit
    
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
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,            // Interrupt flags
    .dma_buf_count = 4,               // Number of DMA buffers
    .dma_buf_len = 1024,              // Length of each DMA buffer
    .use_apll = false,                // Use APLL clock
    .tx_desc_auto_clear = false,      // TX desc auto-clear option
    .fixed_mclk = 0                   // MCLK fixed value (if any)
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
    .intr_alloc_flags = 0,            // Interrupt flags
    .dma_buf_count = 25,               // Number of DMA buffers
    .dma_buf_len = 35,              // Length of each DMA buffer
    .use_apll = false,                // Use APLL clock
    .tx_desc_auto_clear = false,      // TX desc auto-clear option
    .fixed_mclk = 0                   // MCLK fixed value (if any)
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
