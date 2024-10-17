#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <driver/i2s.h>

#define I2S_WS 15
#define I2S_SD 32
#define I2S_SCK 14
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE (8000)
#define I2S_SAMPLE_BITS (16)
#define I2S_READ_LEN (16 * 1024)
#define RECORD_TIME (10) // Seconds
#define I2S_CHANNEL_NUM (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)
#define HEADER_SIZE 44

const char* ssid = "Fedooora";     // Replace with your WiFi SSID
const char* password = "24942494"; // Replace with your WiFi password
const char filename[] = "/recording.wav";
WebServer server(80);

File file;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS initialization failed!");
    while (1) yield();
  }else {
  Serial.println("SPIFFS mounted successfully.");
}

  SPIFFS.remove(filename);
  file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Write WAV header placeholder
  byte header[HEADER_SIZE];
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, HEADER_SIZE);

  // Initialize I2S for recording
  i2sInit();
  recordAudio();

  // Start the web server
  server.on("/download", HTTP_GET, handleDownload);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
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
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = true
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
void recordAudio() {
  size_t bytesRead;
  char* i2s_read_buff = (char*)calloc(I2S_READ_LEN, sizeof(char));
  int flash_wr_size = 0;
  Serial.println(" *** Recording Start *** ");
  delay(1000);

  // Amplification factor (increase or decrease this value as needed)
  const float gain = 4.0; // Try a higher value for more noticeable amplification.

  while (flash_wr_size < FLASH_RECORD_SIZE) {
    // Read data from I2S
    i2s_read(I2S_PORT, (void*)i2s_read_buff, I2S_READ_LEN, &bytesRead, portMAX_DELAY);

    // Apply gain to each sample (16-bit signed samples)
    int16_t* samples = (int16_t*)i2s_read_buff;
    int sampleCount = bytesRead / sizeof(int16_t);
    for (int i = 0; i < sampleCount; ++i) {
      int32_t amplified = (int32_t)(samples[i] * gain);

      // Ensure that the amplified value stays within the range of int16_t (-32768 to 32767)
      if (amplified > 32767) {
        amplified = 32767;
      } else if (amplified < -32768) {
        amplified = -32768;
      }

      samples[i] = (int16_t)amplified;
    }

    // Write the amplified data to the file
    file.write((const byte*)samples, bytesRead);
    flash_wr_size += bytesRead;
  }

  file.close();
  File recordedFile = SPIFFS.open(filename, FILE_READ);
  if (recordedFile) {
    Serial.printf("Recorded file size: %d bytes\n", recordedFile.size());
    recordedFile.close();
  } else {
    Serial.println("Failed to open the recorded file!");
  }

  free(i2s_read_buff);
  Serial.println(" *** Recording Finished *** ");
}

//void recordAudio() {
//  size_t bytesRead;
//  char* i2s_read_buff = (char*)calloc(I2S_READ_LEN, sizeof(char));
//
//  int flash_wr_size = 0;
//  Serial.println(" *** Recording Start *** ");
//
//  while (flash_wr_size < FLASH_RECORD_SIZE) {
//    i2s_read(I2S_PORT, (void*)i2s_read_buff, I2S_READ_LEN, &bytesRead, portMAX_DELAY);
//  file.write((const byte*)i2s_read_buff, bytesRead);
//  flash_wr_size += bytesRead;
//
//  // Add debug statements
// // Serial.printf("Bytes read: %d, Total written: %d\n", bytesRead, flash_wr_size);
//  //Serial.printf("Recording... %d%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
//  }
//
//  file.close();
//  File recordedFile = SPIFFS.open(filename, FILE_READ);
//if (recordedFile) {
//  Serial.printf("Recorded file size: %d bytes\n", recordedFile.size());
//  recordedFile.close();
//} else {
//  Serial.println("Failed to open the recorded file!");
//}
//  free(i2s_read_buff);
//  Serial.println(" *** Recording Finished *** ");
//}

void wavHeader(byte* header, int wavSize) {
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize = wavSize + HEADER_SIZE - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;
  header[21] = 0x00;
  header[22] = 0x01;
  header[23] = 0x00;
  header[24] = (byte)(I2S_SAMPLE_RATE & 0xFF);
  header[25] = (byte)((I2S_SAMPLE_RATE >> 8) & 0xFF);
  header[26] = (byte)((I2S_SAMPLE_RATE >> 16) & 0xFF);
  header[27] = (byte)((I2S_SAMPLE_RATE >> 24) & 0xFF);
  unsigned int byteRate = I2S_SAMPLE_RATE * I2S_CHANNEL_NUM * (I2S_SAMPLE_BITS / 8);
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = (byte)((byteRate >> 16) & 0xFF);
  header[31] = (byte)((byteRate >> 24) & 0xFF);
  header[32] = I2S_CHANNEL_NUM * (I2S_SAMPLE_BITS / 8);
  header[33] = 0x00;
  header[34] = I2S_SAMPLE_BITS;
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);
}

void handleDownload() {
  File audioFile = SPIFFS.open(filename, FILE_READ);
  if (!audioFile) {
    server.send(404, "text/plain", "File not found!");
    return;
  }

  server.streamFile(audioFile, "audio/wav");
  audioFile.close();
}
