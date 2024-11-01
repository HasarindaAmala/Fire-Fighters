#include <driver/i2s.h>
#include <FS.h>          // Include the SPIFFS library
#include <SPIFFS.h>     // Include SPIFFS for file storage

float alpha = 0.2;  // Adjust alpha for cutoff frequency (0 < alpha < 1)
int32_t prev_sample = 0;  // For high-pass filter
int32_t filtered_sample = 0;  // For storing the filtered value

#define NOISE_THRESHOLD 0

#define I2S_WS 15
#define I2S_SD 22
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0

#define SAMPLE_BUFFER_SIZE 512
const int SAMPLE_RATE =44100;
#define BUTTON_PIN 34
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_RIGHT

int32_t sBuffer[SAMPLE_BUFFER_SIZE];
int32_t recordedSamples[SAMPLE_BUFFER_SIZE]; // Array to store recorded samples
int sampleIndex = 0; // Index to keep track of stored samples

const char* FILENAME = "/recording.wav"; // WAV file name

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Setup I2S ...");

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

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

  if (digitalRead(BUTTON_PIN) == LOW) { // Button pressed
    if (result == ESP_OK) {
      int samples_read = bytesIn / sizeof(sBuffer[0]);
      if (samples_read > 0) {
        for (int i = 0; i < samples_read; ++i) {
          filtered_sample = alpha * sBuffer[i] + (1 - alpha) * filtered_sample;
          if (abs(filtered_sample) > NOISE_THRESHOLD) {
            Serial.println(filtered_sample);
          }

          // Store the sample if there is space in the recordedSamples array
          if (sampleIndex < SAMPLE_BUFFER_SIZE) {
            recordedSamples[sampleIndex++] = filtered_sample; // Store the filtered sample
          }
        }
      }
    } else {
      Serial.println("Error: i2s_read() failed");
    }
  } else { // Button released
    if (sampleIndex > 0) { // Only save if there are recorded samples
      saveWavFile(FILENAME, recordedSamples, sampleIndex);
      sendFileOverSerial();
      sampleIndex = 0; // Reset sample index for the next recording
    }
  }
}

// Function to save recorded samples as a WAV file
void saveWavFile(const char* filename, int32_t* samples, int numSamples) {
    File file = SPIFFS.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    // Write WAV file header
    const uint8_t riff[] = {'R', 'I', 'F', 'F'};
    const uint8_t wave[] = {'W', 'A', 'V', 'E'};
    const uint8_t fmt[] = {'f', 'm', 't', ' '};
    const uint8_t data[] = {'d', 'a', 't', 'a'};

    file.write(riff, sizeof(riff)); // Chunk ID
    file.write((uint8_t*)&numSamples, sizeof(uint32_t)); // Placeholder for chunk size
    file.write(wave, sizeof(wave)); // Format
    file.write(fmt, sizeof(fmt)); // Subchunk1 ID

    uint32_t subchunk1Size = 16; // Subchunk1 size
    file.write((uint8_t*)&subchunk1Size, sizeof(subchunk1Size)); // Subchunk1 size
    uint16_t audioFormat = 1; // PCM format
    file.write((uint8_t*)&audioFormat, sizeof(audioFormat)); // Audio format
    uint16_t numChannels = 1; // Mono
    file.write((uint8_t*)&numChannels, sizeof(numChannels)); // Number of channels
    file.write((uint8_t*)&SAMPLE_RATE, sizeof(uint32_t)); // Sample rate

    uint32_t byteRate = SAMPLE_RATE * numChannels * sizeof(int32_t); // Byte rate
    file.write((uint8_t*)&byteRate, sizeof(byteRate)); // Byte rate
    uint16_t blockAlign = numChannels * sizeof(int32_t); // Block align
    file.write((uint8_t*)&blockAlign, sizeof(blockAlign)); // Block align
    uint16_t bitsPerSample = sizeof(int32_t) * 8; // Bits per sample
    file.write((uint8_t*)&bitsPerSample, sizeof(bitsPerSample)); // Bits per sample
    file.write(data, sizeof(data)); // Subchunk2 ID

    uint32_t subchunk2Size = numSamples * sizeof(int32_t); // Subchunk2 size
    file.write((uint8_t*)&subchunk2Size, sizeof(subchunk2Size)); // Subchunk2 size

    // Write the recorded samples
    for (int i = 0; i < numSamples; i++) {
        int16_t sample = (int16_t)(samples[i] >> 16); // Convert 32-bit sample to 16-bit
        file.write((uint8_t*)&sample, sizeof(sample));
    }

    // Seek back to the chunk size position and write the size
    file.seek(4, SeekSet);
    uint32_t chunkSize = 36 + subchunk2Size;
    file.write((uint8_t*)&chunkSize, sizeof(chunkSize));

    file.close();
}



// Function to send WAV file over serial
void sendFileOverSerial() {
  File file = SPIFFS.open(FILENAME, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}

void i2s_install() {
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_MIC_CHANNEL,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
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
